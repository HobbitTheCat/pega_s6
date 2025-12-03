#include "Protocol/proto_io.h"

#include <stdlib.h>
#include <string.h>

void rx_init(rx_state_t* rx) {
    memset(rx, 0, sizeof(rx_state_t));
    rx_reset_to_header(rx, 0);
}

void rx_reset_to_header(rx_state_t* rx, const uint16_t remain) {
    rx->have = remain;
}

void tx_init(tx_state_t* tx) {
    tx->head = tx->tail = NULL;
    tx->queued = 0;
    tx->high_watermark = 1<<20;
}

void tx_free_queue(tx_state_t* tx) {
    out_chunk_t* chunk = tx->head;
    while (chunk) {
        out_chunk_t* next = chunk->next;
        free(chunk->data);
        free(chunk);
        chunk = next;
    }
    tx->head = tx->tail = NULL;
    tx->queued = 0;
}

int rx_try_extract_frame(rx_state_t* rx, header_t* out_header, uint8_t** out_payload, uint16_t* out_payload_len) {
    if (rx->have < sizeof(header_t)) return 0;
    header_t header;
    const int r = try_peek_header(rx->buf, rx->have, &header); // возможна оптимизация при которой парсинг происходит только один раз при получении header
    if (r == -1) return 0;
    if (r == -2) return -1;
    const uint32_t total_needed_length = header.payload_length + sizeof(header_t);
    if (total_needed_length > sizeof(rx->buf)) return -1;
    if (rx->have < total_needed_length) return 0;
    *out_header = header;
    *out_payload = rx->buf + sizeof(header_t);
    *out_payload_len = header.payload_length;
    const uint32_t remain = rx->have - total_needed_length;
    if (remain) memmove(rx->buf, rx->buf + total_needed_length, remain);
    rx->have = remain;
    return 1;
}
