#ifndef SPSC_BUFFER_H
#define SPSC_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

#define BUFFER_SUCCESS    0
#define BUFFER_EINVAL    -1  // Incorrect parameters
#define BUFFER_ENOMEM    -2  // Not enough memory
#define BUFFER_EFULL     -3  // The buffer is full
#define BUFFER_EEMPTY    -4  // The buffer is empty

typedef struct {
    void *buffer;
    size_t element_size;
    size_t capacity;
    size_t mask;

    _Atomic size_t head;
    _Atomic size_t tail;

    char padding[64 - sizeof(_Atomic size_t) * 2];
} ring_buffer_t;

bool is_power_of_two(size_t n);

int buffer_init(ring_buffer_t* rb, size_t capacity, size_t element_size);

void buffer_destroy(ring_buffer_t* rb);
void buffer_free(ring_buffer_t* rb);

bool buffer_is_empty(ring_buffer_t* rb);
bool buffer_is_full(ring_buffer_t* rb);
size_t buffer_size(const ring_buffer_t* rb);

int buffer_push(ring_buffer_t* rb, const void* data);
int buffer_pop(ring_buffer_t* rb, void* data);

int buffer_peek(ring_buffer_t* rb, void* data);

#endif //SPSC_BUFFER_H
