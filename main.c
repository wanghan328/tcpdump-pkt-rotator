#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "ring_buffer.h"
#include "packet.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define LOG_FILE_SIZE (1024 * 1024 + PACKET_LENGTH_MAX)
#define RING_BUF_SIZE (1.2 * PACKET_LENGTH_MAX)

static ring_buffer_t *rb;

static header_t header;
static packet_t *packet;
static int packet_cnt;

static char block[LOG_FILE_SIZE];
static size_t offset = 0;

static int output_index = 1;
static int limit = 5;
/*
 * ret = 0, OK
 * ret = 1, finish, no more data
 */
int read_input() {
    static char buffer[4096];
    ssize_t length = read(0, buffer, sizeof(buffer));
    if (length == -1) {
        int err = errno;
        printf("[input ] read length is -1, err=%d.\n", err);
        if (err == EINTR) {
            return 0;
        }
        if (err == EAGAIN) {
            usleep(1000);
            return 0;
        }
        // should finish read
        return 1;
    } else if (length == 0) {
        printf("[input ] read length is 0.\n");
        // should finish read
        return 1;
    } else {
        size_t rb_remain = ring_buffer_capacity(rb) - ring_buffer_length(rb);
        assert(rb_remain >= (size_t)length);
        size_t write_length = ring_buffer_write(rb, buffer, length);
        assert (write_length == (size_t)length);
        return 0;
    }
}

int write_file(char *filename, void *buffer, size_t length, char *delete) {
    printf("[file  ] writing=%s, deleting=%s\n", filename, delete);
    // open file
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int output_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
    assert(output_fd >= 0);

    for (;;) {
        ssize_t write_length = write(output_fd, buffer, length);
        if (write_length == -1) {
            int err = errno;
            printf("[input ] write length is -1, err=%d.\n", err);
            if (err == EINTR) {
                continue;
            }
            close(output_fd);
            return 1;
        }
        assert(write_length == (ssize_t)length); // will fail if no space left on disk, then we exit.
        close(output_fd);
        break;
    } 

    if (delete) {
        unlink(delete);
    }
    return 0;
}

int read_header() {
    if (packet) {
        return 0;
    }
    size_t rb_length = ring_buffer_length(rb);
    if (rb_length < sizeof(header)) {
        return 1;
    }
    size_t read_length = ring_buffer_read(rb, &header, sizeof(header));
    assert(read_length == sizeof(header));
    assert(header.magic == PACKET_HEADER_MAGIC);
    assert(header.packet_size <= PACKET_LENGTH_MAX);

    printf("[packet] header: version=%d.%u, packet_size=%u, frame_type=%u\n", 
        header.ver_major, header.ver_minor, header.packet_size, header.frame_type);
    write_file("output.00000", &header, sizeof(header), NULL);

    packet = malloc(sizeof(*packet) + header.packet_size);
    assert(packet);

    return 0;
}

int read_packet() {
    size_t rb_length = ring_buffer_length(rb);

    if (packet == NULL) {
        read_header();
        if (packet == NULL) {
            return 1;
        }
        rb_length = ring_buffer_length(rb);
    }

    if (rb_length < sizeof(*packet)) {
        return 2;
    }

    size_t peek_length = ring_buffer_peek(rb, packet, sizeof(*packet));
    assert(peek_length == sizeof(*packet));
    assert(packet->data_size <= header.packet_size);
    size_t packet_length = (sizeof(*packet) + packet->data_size);
    if (rb_length < packet_length) {
        return 3;
    }

    size_t read_length = ring_buffer_read(rb, packet, packet_length);
    assert(read_length == packet_length);
    packet_cnt++;
    //printf("[packet] packet: length=%d\n", (int)packet_length);
    return 0;
}

void output_block() {
    char filename[64];
    char delete[64];
    sprintf(filename, "output.%05d", output_index);
    sprintf(delete, "output.%05d", output_index - limit);
    write_file(filename, block, offset, (output_index > limit) ? delete : NULL);
    output_index++;
    offset = 0;
}

int write_block() {
    size_t packet_length = (sizeof(*packet) + packet->data_size);
    if (packet_length + offset > LOG_FILE_SIZE) {
        output_block();
    }

    memcpy(block + offset, packet, packet_length);
    offset += packet_length;
    return 0;
}

void signal_install() {
    signal(SIGINT, SIG_IGN);
}

#define OPTSTR "n:sh?"
static int parse_opt(int argc, char *argv[]) {
    int ch;
    while (-1 != (ch = getopt(argc, argv, OPTSTR))) {
        switch (ch) {
        case 'n': limit = MAX(1, atoi(optarg));     break;
        case 's': signal_install();                 break;
        case 'h':
        case '?': printf("usage:         argv[0] -n <number> -s\n"
                    "-n <number>    keep <number> blocks at most, default 5.\n"
                    "-s             ommit signal CTRL-C, default not ommit.\n");
        default : exit(0);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    parse_opt(argc, argv);
    rb = ring_buffer_new(RING_BUF_SIZE);

    for (;;) {
        int finish = read_input();
        int ret = 0;
        do {
            ret = read_packet();
            if (ret == 0) {
                // drop to block
                write_block();
            }
        } while(ret == 0);
        
        if (finish) {
            size_t rb_length = ring_buffer_length(rb);
            output_block();
            printf("[main  ] finished, packet_cnt=%d, rb_left=%d\n", packet_cnt, (int)rb_length);
            // assert(rb_length == 0); // will fail if read from courrpted file.
            break;
        }
    }
    return 0;
}