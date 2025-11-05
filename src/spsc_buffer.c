#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

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

static inline bool is_power_of_two(const size_t n) {
    return (n & (n - 1)) == 0;
}

int buffer_init(ring_buffer_t* rb, size_t capacity, size_t element_size) {
    if (!rb) return BUFFER_EINVAL;
    if (capacity == 0 || element_size == 0) return BUFFER_EINVAL;
    if (!is_power_of_two(capacity)) return BUFFER_EINVAL;

    rb->buffer = malloc(capacity * element_size);
    if (!rb->buffer) return BUFFER_ENOMEM;

    rb->element_size = element_size;
    rb->capacity = capacity;
    rb->mask = capacity - 1;

    atomic_init(&rb->head, 0);
    atomic_init(&rb->tail, 0);

    return BUFFER_SUCCESS;
}

void buffer_destroy(ring_buffer_t* rb) {
    if (rb && rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
}

void buffer_free(ring_buffer_t* rb) {
    if (rb) {
        buffer_destroy(rb);
        free(rb);
    }
}

bool buffer_is_empty(ring_buffer_t* rb) {
    if (!rb) return true;

    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return (head == tail);
}

bool buffer_is_full(ring_buffer_t* rb) {
    if (!rb) return false;

    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return (((head + 1) & rb->mask) == tail);
}

size_t buffer_size(const ring_buffer_t *rb) {
    if (!rb) return 0;

    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);
    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);
    return ((head - tail) & rb->mask);
}

int buffer_push(ring_buffer_t *rb, const void *data) {
    if (!rb || !data) return BUFFER_EINVAL;

    const size_t head = atomic_load_explicit(&rb->head, memory_order_relaxed);
    const size_t next_head = (head + 1) & rb->mask;

    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_acquire);

    if (next_head == tail) return BUFFER_EFULL;

    void *slot = (char*)rb->buffer + (head & rb->mask) * rb->element_size;
    memcpy(slot, data, rb->element_size);

    atomic_store_explicit(&rb->head, next_head, memory_order_release);
    return BUFFER_SUCCESS;
}

int buffer_pop(ring_buffer_t *rb, void *data) {
    if (!rb || !data) return BUFFER_EINVAL;

    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    if (tail == head) return BUFFER_EEMPTY;

    const void *slot = (char*)rb->buffer + (tail & rb->mask) * rb->element_size;
    memcpy(data, slot, rb->element_size);

    const size_t next_tail = (tail + 1) & rb->mask;
    atomic_store_explicit(&rb->tail, next_tail, memory_order_release);

    return BUFFER_SUCCESS;
}

int buffer_peek(ring_buffer_t *rb, void *data) {
    if (!rb || !data) return BUFFER_EINVAL;

    const size_t tail = atomic_load_explicit(&rb->tail, memory_order_relaxed);
    const size_t head = atomic_load_explicit(&rb->head, memory_order_acquire);

    if (tail == head) return BUFFER_EEMPTY;

    const void *slot = (char*)rb->buffer + (tail & rb->mask) * rb->element_size;
    memcpy(data, slot, rb->element_size);

    return BUFFER_SUCCESS;
}
