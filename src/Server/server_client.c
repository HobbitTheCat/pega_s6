#include "Server/server.h"
#include "Protocol/proto_io.h"
#include "SupportStructure/fd_map.h"

#include <sys/random.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>

static const char TOKEN_ALPHABET[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789";

#define TOKEN_ALPHABET_LEN (sizeof(TOKEN_ALPHABET) - 1)

int generate_reconnection_token(char* buf, const size_t len) {
    if (len < 2) return -1;

    for (size_t i = 0; i < len - 1; i++) {
        uint8_t rnd;
        if (getrandom(&rnd, 1, 0) != 1) return -1;
        buf[i] = TOKEN_ALPHABET[rnd % TOKEN_ALPHABET_LEN];
    }

    buf[len - 1] = '\0';
    return 0;
}

int enqueue_message(server_t* server, server_conn_t* conn, uint8_t* buffer, const uint32_t total_length) {
    if (conn->tx.queued + total_length > conn->tx.high_watermark) return -1;

    out_chunk_t* chunk = malloc(sizeof(*chunk));
    if (!chunk) return -1;

    chunk->data = buffer;
    chunk->len = total_length;
    chunk->off = 0;
    chunk->next = NULL;

    if (!conn->tx.tail) conn->tx.head = conn->tx.tail = chunk;
    else {conn->tx.tail->next = chunk; conn->tx.tail = chunk;}

    conn->tx.queued += total_length;
    handle_write(server, conn);

    if (conn->tx.head && conn->want_epollout == 0) {
        mod_epoll_event(server->epoll_fd, conn->fd,
            EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLOUT,
            fd_map_get(server->response, conn->fd));
        conn->want_epollout = 1;
    }
    return 0;
}

int send_packet(server_t* server, server_conn_t* conn, const uint8_t type, const void* payload, const uint16_t payload_length){
    const uint32_t total_length = sizeof(header_t) + payload_length;
    if (total_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* buff = malloc(total_length);
    if (!buff) return -1;

    header_t* header = (header_t*)buff;
    if (protocol_header_encode(header, type, payload_length) < 0) {free(buff); return -1;}

    if (payload_length > 0) memcpy(buff + sizeof(header_t), payload, payload_length);
    return enqueue_message(server, conn, buff, total_length);
}

uint32_t get_new_client_id(server_t* server) {
    int new_client_id = fd_map_get_first_null_id(server->registered_players, (int)server->next_player_id);
    if (new_client_id == -1) new_client_id = (int)server->next_player_id++;
    return new_client_id;
}

int register_client(server_t* server, server_conn_t* conn) {
    const uint32_t new_client_id = get_new_client_id(server);
    server_player_t* player = malloc(sizeof(*player));
    if (!player) return -1;
    memset(player, 0, sizeof(*player));

    player->conn = conn;
    player->user_id = new_client_id;
    player->session_id = 0;
    player->state = PLAYER_STATE_LOBBY;

    const size_t token_len = 32;
    player->reconnection_token = malloc(token_len);
    if (!player->reconnection_token) {free(player); return -1;}
    if (generate_reconnection_token(player->reconnection_token, token_len) != 0) {
        free(player->reconnection_token);
        free(player);
        return -1;
    }

    fd_map_set(server->registered_players, (int)new_client_id, player);

    conn->state = CONN_STATE_CONNECT;
    conn->player = player;
    return 0;
}


int disconnect_client(server_t* server, server_conn_t* conn) {
    if (!conn->player) return -1;
    conn->player->conn = NULL;
    conn->player->state = PLAYER_STATE_DISCONNECT;
    if (conn->player->session_id != 0) {
        send_system_message_to_session(server, conn->player->session_id, conn->player->user_id, USER_LEFT);
    }
    conn->player = NULL;
    return 0;
}

int reconnect_client(server_t* server, server_conn_t* conn, const uint32_t client_id, const char* reconnection_token) {
    server_player_t* player = fd_map_get(server->registered_players, (int)client_id);
    if (!player) return -1; // player not found
    if (!player->reconnection_token || !reconnection_token ||
    strcmp(player->reconnection_token, reconnection_token) != 0) {
        printf("%s != %s\n", player->reconnection_token, reconnection_token);
        return -2;
    }
    if (player->conn && player->conn != conn) {cleanup_connection(server, player->conn);}
    if (conn->player && conn->player != player) {conn->player->state = PLAYER_STATE_DISCONNECT; conn->player->conn = NULL;}
    conn->state = CONN_STATE_CONNECT;
    if (player->session_id != 0) player->state = PLAYER_STATE_IN_GAME;
    else player->state = PLAYER_STATE_LOBBY;
    player->conn = conn;
    conn->player = player;
    return 0;
}

int unregister_client(server_t* server, const uint32_t player_id) {
    server_player_t* player = fd_map_get(server->registered_players, (int)player_id);
    if (!player) return -1;
    if (player->conn) {
        player->conn->state = CONN_STATE_HANDSHAKE;
        player->conn->player = NULL;
    }
    if (player->session_id != 0) {
        send_system_message_to_session(server, player->session_id, player->user_id, USER_UNREGISTER);
    }
    fd_map_remove(server->registered_players, (int)player_id);
    free(player->reconnection_token);
    free(player);
    return 0;
}
