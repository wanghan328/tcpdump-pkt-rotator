#ifndef RING_BUFFER_H
#define RING_BUFFER_H

struct ring_buffer;
typedef struct ring_buffer ring_buffer_t;

ring_buffer_t *ring_buffer_new(size_t capacity);
void ring_buffer_del(ring_buffer_t *rb);

size_t ring_buffer_capacity(ring_buffer_t *rb);
size_t ring_buffer_length(ring_buffer_t *rb);

size_t ring_buffer_write(ring_buffer_t *rb, void *src, size_t limit);
size_t ring_buffer_peek(ring_buffer_t *rb, void *dest, size_t limit);
size_t ring_buffer_read(ring_buffer_t *rb, void *dest, size_t limit);

#endif