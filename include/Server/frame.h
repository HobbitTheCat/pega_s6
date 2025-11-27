#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include"Protocol/protocol.h"

#define SP_MAX_FRAME 4096
typedef enum {R_HEADER, R_PAYLOAD} recv_state_t;

typedef struct out_chunk{
    uint8_t* data;
    uint32_t len;
    uint32_t off;
    struct out_chunk *next;
} out_chunk_t;

typedef struct {
    recv_state_t state;
    uint8_t buf[SP_MAX_FRAME];
    uint32_t have;

    uint32_t read_pos, write_pos;

    uint32_t need;
} rx_state_t;

typedef struct {
    struct out_chunk *head, *tail;
    size_t queued;
    size_t high_watermark;
    int epollout;
} tx_state_t;

typedef struct {
    int fd;
    uint32_t user_id;
    uint32_t session_id;
    packet_type_t packet_type;

    rx_state_t rx;
    tx_state_t tx;
} frame_t;


void init_client(frame_t* frame, int fd);
void reset_to_header(frame_t* frame);

void free_tx_queue(tx_state_t* tx);

void set_payload_from_header(frame_t* frame, const header_t* header);


#endif //FRAME_H
