#include "Server/server_conn.h"

#include <string.h>

void server_conn_init(server_conn_t* conn, const int fd) {
    memset(conn, 0, sizeof(*conn));
    conn->fd = fd;

    rx_init(&conn->rx);
    tx_init(&conn->tx);

    conn->state = CONN_STATE_INIT;
    conn->player = NULL;
    conn->want_epollout = 0;
}

void server_conn_free_tx(server_conn_t* conn) {
    tx_free_queue(&conn->tx);
    conn->want_epollout = 0;
}
