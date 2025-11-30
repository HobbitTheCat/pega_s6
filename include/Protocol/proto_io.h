#ifndef PROTO_IO_H
#define PROTO_IO_H

#include <stdint.h>
#include "Protocol/protocol.h"

typedef enum {R_HEADER, R_PAYLOAD} recv_state_t;

typedef struct out_chunk {
    uint8_t* data;
    uint32_t len;
    uint32_t off;
    struct out_chunk* next;
} out_chunk_t;

typedef struct {
    recv_state_t state;
    uint8_t buf[PROTOCOL_MAX_SIZE];
    uint32_t have;

    uint32_t read_pos, write_pos;
    uint32_t need;
} rx_state_t;

typedef struct {
    struct out_chunk *head, *tail;
    size_t queued;
    size_t high_watermark;
} tx_state_t;

void rx_init(rx_state_t* rx);
void rx_reset_to_header(rx_state_t* rx, uint32_t remain);

void rx_set_payload(rx_state_t* rx, uint32_t payload_len);

void tx_init(tx_state_t* tx);
void tx_free_queue(tx_state_t* tx);

#endif //PROTO_IO_H
