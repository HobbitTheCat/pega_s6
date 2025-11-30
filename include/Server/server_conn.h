#ifndef SERVER_CONN_H
#define SERVER_CONN_H

#include "Protocol/proto_io.h"

typedef struct {
    int fd;
    uint32_t user_id;
    uint32_t session_id;
    packet_type_t packet_type;

    rx_state_t rx;
    tx_state_t tx;

    int want_epollout;
} server_conn_t;

void server_conn_init(server_conn_t *conn, int fd);
void server_conn_reset_to_header(server_conn_t *conn, uint32_t remain);
void server_conn_free_tx(server_conn_t* conn);
void server_conn_set_payload_from_header(server_conn_t *conn, const header_t* header);

#endif //SERVER_CONN_H
