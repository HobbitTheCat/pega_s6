#ifndef SERVER_PROTO_H
#define SERVER_PROTO_H

#include <stdint.h>
#include "Protocol/proto_io.h"

typedef enum {
    CONN_STATE_HANDSHAKE,
    CONN_STATE_LOBBY,
    CONN_STATE_IN_GAME,
    CONN_STATE_DISCONNECT
} conn_state_t;

typedef struct {
    int fd;

    conn_state_t state;
    uint32_t user_id;
    uint32_t session_id;

    rx_state_t rx;
    tx_state_t tx;

    int want_epollout;
} server_conn_t;

void server_conn_init(server_conn_t* conn, int fd);
void server_conn_free_tx(server_conn_t* conn);

#endif //SERVER_PROTO_H
