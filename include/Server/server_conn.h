#ifndef SERVER_PROTO_H
#define SERVER_PROTO_H

#include <stdint.h>
#include "Protocol/proto_io.h"
#include "SupportStructure/session_bus.h"

typedef enum {
    PLAYER_STATE_LOBBY,
    PLAYER_STATE_IN_GAME,
    PLAYER_STATE_DISCONNECT
} player_state_t;

typedef enum {
    CONN_STATE_INIT,
    CONN_STATE_HANDSHAKE,
    CONN_STATE_CONNECT
} conn_state_t;


struct server_player_s;
struct server_conn_s;

typedef struct server_player_s {
    player_state_t state;

    uint32_t user_id;
    uint32_t session_id;

    char* reconnection_token;
    struct server_conn_s* conn; // NULL if disconnected
} server_player_t;

typedef struct server_conn_s {
    int fd;
    conn_state_t state;

    rx_state_t rx;
    tx_state_t tx;

    int want_epollout;

    server_player_t* player; // NULL before registration/reconnect
} server_conn_t;

typedef struct {
    session_bus_t* bus;
    uint8_t is_visible;
    size_t capacity;
    size_t number_players;
} server_session_t;

void server_conn_init(server_conn_t* conn, int fd);
void server_conn_free_tx(server_conn_t* conn);

#endif //SERVER_PROTO_H
