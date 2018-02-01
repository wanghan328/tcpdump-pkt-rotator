#ifndef PACKET_H
#define PACKET_H

#define PACKET_HEADER_MAGIC (0xA1B2C3D4)
#define PACKET_LENGTH_MAX   (256 * 1024)

#pragma pack(1)

typedef struct {
    unsigned int magic; // 0xA1B2C3D4
    unsigned short ver_major;
    unsigned short ver_minor;
    unsigned int time_zone;
    unsigned int time_resolution;
    unsigned int packet_size;
    unsigned int frame_type;
} header_t;

typedef struct {
    unsigned int second;
    unsigned int usecond;
    unsigned int data_size;
    unsigned int real_size;
    unsigned char data[0];
} packet_t;

#pragma pack()

#endif