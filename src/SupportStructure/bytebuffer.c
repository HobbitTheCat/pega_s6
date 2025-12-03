#include "SupportStructure/bytebuffer.h"

#include <stdlib.h>
#include <string.h>

bytebuffer_t* bytebuffer_init(const size_t capacity) {
    bytebuffer_t* buf = malloc(sizeof(bytebuffer_t));
    if (!buf) return NULL;

    buf->data = malloc(capacity);
    if (!buf->data) {free(buf); return NULL;}

    buf->capacity = capacity;
    buf->position = 0;
    buf->owner = 1;
    return buf;
}

bytebuffer_t* bytebuffer_wrap(uint8_t* data, const size_t length) {
    bytebuffer_t* buf = malloc(sizeof(bytebuffer_t));
    if (!buf) return NULL;

    buf->data = data;
    buf->capacity = length;
    buf->position = 0;
    buf->owner = 0;
    return buf;
}
void bytebuffer_clean(bytebuffer_t* buffer) {
    if (!buffer) return;
    if (buffer->owner == 1 && buffer->data) {
        free(buffer->data);
    }
    free(buffer);
}

uint8_t* bytebuffer_get_data(bytebuffer_t* buffer) {
    if (!buffer) return NULL;
    buffer->owner = 0;
    return buffer->data;
}
void bytebuffer_reset(bytebuffer_t* buffer) {
    buffer->position = 0;
}
size_t bytebuffer_remaining(const bytebuffer_t* buffer) {
    return buffer->capacity - buffer->position;
}
void bytebuffer_set_cursor(bytebuffer_t* buffer,const int position) {
    if (position < 0 || position > buffer->capacity) return;
    buffer->position = position;
}

int bytebuffer_put_uint8(bytebuffer_t *buf, const uint8_t value) {
    if (buf->position >= buf->capacity) return -1;
    buf->data[buf->position++] = value;
    return 0;
}
int bytebuffer_put_uint16(bytebuffer_t *buf, const uint16_t value) {
    if (buf->position + 2 > buf->capacity) return -1;
    memcpy(buf->data + buf->position, &value, sizeof(uint16_t));
    buf->position += 2;
    return 0;
}
int bytebuffer_put_uint32(bytebuffer_t *buf, const uint32_t value) {
    if (buf->position + 4 > buf->capacity) return -1;
    memcpy(buf->data + buf->position, &value, sizeof(uint32_t));
    buf->position += 4;
    return 0;
}
int bytebuffer_put_bytes(bytebuffer_t *buf, const uint8_t* data,const size_t length) {
    if (buf->position + length > buf->capacity) return -1;
    memcpy(buf->data + buf->position, data, length);
    buf->position += length;
    return 0;
}

int bytebuffer_get_uint8(bytebuffer_t *buf, uint8_t* value) {
    if (buf->position >= buf->capacity) return -1;
    *value = buf->data[buf->position++];
    return 0;
}
int bytebuffer_get_uint16(bytebuffer_t *buf, uint16_t* value) {
    if (buf->position + 2 > buf->capacity) return -1;
    memcpy(value, buf->data + buf->position, 2);
    buf->position += 2;
    return 0;
}
int bytebuffer_get_uint32(bytebuffer_t *buf, uint32_t* value) {
    if (buf->position + 4 > buf->capacity) return -1;
    memcpy(value, buf->data + buf->position, 4);
    buf->position += 4;
    return 0;
}
int bytebuffer_get_bytes(bytebuffer_t *buf, uint8_t* data, const size_t length) {
    if (buf->position + length > buf->capacity) return -1;
    memcpy(data, buf->data + buf->position, length);
    buf->position += length;
    return 0;
}