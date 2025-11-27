#include "Server/frame.h"

#include <string.h>

void init_client(frame_t* frame, const int fd) {
    memset(frame, 0, sizeof(*frame));
    frame->fd = fd;
    reset_to_header(frame);

    frame->tx.head = frame->tx.tail = NULL;
    frame->tx.queued = 0;
    frame->tx.high_watermark = 1 << 20;
    frame->tx.epollout = 0;
}

void reset_to_header(frame_t* frame) {
    frame->rx.state = R_HEADER;
    frame->rx.need = sizeof(header_t);
}

void free_tx_queue(tx_state_t* tx) {
     out_chunk_t* chunk = tx->head;
     while (chunk) {
         out_chunk_t* next = chunk->next;
         free(chunk->data);
         free(chunk);
         chunk = next;
     }
     tx->head = NULL;
     tx->queued = 0;
     tx->epollout = 0;
}

void set_payload_from_header(frame_t* frame, const header_t* header) {
    frame->rx.need = header->length;
    frame->user_id = header->user_id;
    frame->session_id = header->session_id;
    frame->packet_type = header->type;
    frame->rx.state = R_PAYLOAD;
}
