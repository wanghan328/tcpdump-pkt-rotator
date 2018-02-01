#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ring_buffer.h"

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

typedef struct ring_buffer {
	size_t capacity;  // capacity
	size_t read;      // pos of read
	size_t write;     // pos of write
	size_t length;    // current length
	char  *buffer;    // the buffer
} ring_buffer_t;
ring_buffer_t *ring_buffer_new(size_t capacity) {
	void *buffer = NULL;
	ring_buffer_t *rb = NULL;

	assert(capacity);
	assert(capacity <= 1024 * 1024);

	buffer = malloc(capacity);
	assert(buffer);

	rb = malloc(sizeof(ring_buffer_t));
	assert(rb);

	rb->capacity = capacity;
	rb->read   = 0;
	rb->write  = 0;
	rb->length = 0;
	rb->buffer = buffer;
	return rb;
}
void ring_buffer_del(ring_buffer_t *rb) {
	free(rb->buffer);
	free(rb);
}

size_t ring_buffer_capacity(ring_buffer_t *rb) {
	return rb->capacity;
}

size_t ring_buffer_length(ring_buffer_t *rb) {
	return rb->length;
}

size_t ring_buffer_write(ring_buffer_t *rb, void *src, size_t src_size) {
	size_t cap_tail;
	size_t write_tail, wirte_head;

	src_size = MIN(src_size, rb->capacity - rb->length);
	cap_tail = rb->capacity - rb->write;

	write_tail = MIN(src_size, cap_tail);
	if (write_tail) {
		memcpy(rb->buffer + rb->write, src, write_tail);
	}
	wirte_head = src_size - write_tail;
	if (wirte_head) {
		memcpy(rb->buffer + 0, src + write_tail, wirte_head);
	}

	rb->write = (rb->write + src_size) % rb->capacity;
	rb->length += src_size;
	return src_size;
}

size_t ring_buffer_peek(ring_buffer_t *rb, void *dst, size_t dst_size) {
	size_t cap_tail;
	size_t read_tail, read_head;

	dst_size = MIN(dst_size, rb->length);
	cap_tail = rb->capacity - rb->read;

	read_tail = MIN(dst_size, cap_tail);
	if (read_tail) {
		memcpy(dst, rb->buffer + rb->read, read_tail);
	}
	read_head = dst_size - read_tail;
	if (read_head) {
		memcpy(dst + read_tail, rb->buffer + 0, read_head);
	}

	/* peek: do not update read & length */
	// rb->read = (rb-> read + dst_size) % rb->capacity;
	// rb->length -= dst_size;
	return dst_size;
}

size_t ring_buffer_read(ring_buffer_t *rb, void *dst, size_t dst_size) {
	size_t read_size = ring_buffer_peek(rb, dst, dst_size);
	rb->read = (rb-> read + read_size) % rb->capacity;
	rb->length -= read_size;
	return read_size;
}
