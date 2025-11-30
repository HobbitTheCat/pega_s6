#include "Server/server_conn.h"

#include <string.h>

void server_conn_init(server_conn_t* conn, const int fd) {
    memset(conn, 0, sizeof(*conn));
    conn->fd = fd;

    rx_init(&conn->rx);
    tx_init(&conn->tx);

    conn->want_epollout = 0;
}

void server_conn_reset_to_header(server_conn_t *conn, const uint32_t remain) {
    rx_reset_to_header(&conn->rx, remain);
}

void server_conn_free_tx(server_conn_t* conn) {
    tx_free_queue(&conn->tx);
    conn->want_epollout = 0;
}

void server_conn_set_payload_from_header(server_conn_t *conn, const header_t* header) {
    rx_set_payload(&conn->rx, header->length);

    conn->user_id = header->user_id;
    conn->session_id = header->session_id;
    conn->packet_type = header->type;
}
