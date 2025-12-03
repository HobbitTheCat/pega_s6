#ifndef PROTO_IO_H
#define PROTO_IO_H

#include <stddef.h>

#include "Protocol/protocol.h"

typedef struct out_chunk {
    uint8_t* data;
    uint32_t len;
    uint32_t off;
    struct out_chunk* next;
} out_chunk_t;

typedef struct {
    uint8_t buf[PROTOCOL_MAX_SIZE];
    uint32_t have;
} rx_state_t;

typedef struct {
    struct out_chunk *head, *tail;
    size_t queued;
    size_t high_watermark;
} tx_state_t;


void rx_init(rx_state_t* rx);
void rx_reset_to_header(rx_state_t* rx, uint16_t remain);

int rx_try_extract_frame(rx_state_t* rx, header_t* out_header, uint8_t** out_payload, uint16_t* out_payload_len);

void tx_init(tx_state_t* tx);
void tx_free_queue(tx_state_t* tx);

#endif //PROTO_IO_H
