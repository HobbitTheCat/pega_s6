#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t* data;
    size_t capacity;
    size_t position;

    int owner;
} bytebuffer_t;

bytebuffer_t* bytebuffer_init(size_t capacity);
bytebuffer_t* bytebuffer_wrap(uint8_t* data, size_t length);
void bytebuffer_clean(bytebuffer_t* buffer);

void bytebuffer_set_cursor(bytebuffer_t* buffer, int position);
uint8_t* bytebuffer_get_data(bytebuffer_t* buffer);
void bytebuffer_reset(bytebuffer_t* buffer);
size_t bytebuffer_remaining(const bytebuffer_t* buffer);

int bytebuffer_put_uint8(bytebuffer_t *buf, uint8_t value);
int bytebuffer_put_uint16(bytebuffer_t *buf, uint16_t value);
int bytebuffer_put_uint32(bytebuffer_t *buf, uint32_t value);
int bytebuffer_put_bytes(bytebuffer_t *buf, const uint8_t* data, size_t length);

int bytebuffer_get_uint8(bytebuffer_t *buf, uint8_t* value);
int bytebuffer_get_uint16(bytebuffer_t *buf, uint16_t* value);
int bytebuffer_get_uint32(bytebuffer_t *buf, uint32_t* value);
int bytebuffer_get_bytes(bytebuffer_t *buf, uint8_t* data, size_t length);

#endif //BYTEBUFFER_H
