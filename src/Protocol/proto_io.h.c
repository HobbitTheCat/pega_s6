#include <stdlib.h>
#include <string.h>

#include "Protocol/proto_io.h"

void rx_init(rx_state_t* rx) {
    memset(rx, 0, sizeof(*rx));
    rx_reset_to_header(rx, 0);
}

void rx_reset_to_header(rx_state_t* rx, const uint32_t remain) {
    rx->state = R_HEADER;
    rx->need = sizeof(header_t);
    rx->have = remain;
    rx->read_pos = rx->write_pos = 0;
}

void rx_set_payload(rx_state_t* rx, const uint32_t payload_len) {
    rx->state = R_PAYLOAD;
    rx->need = payload_len;
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