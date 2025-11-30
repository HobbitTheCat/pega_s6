#include "Server/server.h"
#include "Protocol/proto_io.h"
#include "SupportStructure/fd_map.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>

int enqueue_message(const server_t* server, server_conn_t* conn, const uint8_t* buf, const uint32_t length) {
    if (conn->tx.queued + length > conn->tx.high_watermark) return -1;

    out_chunk_t* chunk = malloc(sizeof(*chunk));
    if (!chunk) return -1;
    chunk->data = malloc(length);
    if (!chunk->data) {free(chunk); return -1;}
    memcpy(chunk->data, buf, length);
    chunk->len = length;
    chunk->off = 0;
    chunk->next = NULL;

    if (!conn->tx.tail) conn->tx.head = conn->tx.tail = chunk;
    else {conn->tx.tail->next = chunk; conn->tx.tail = chunk;}

    conn->tx.queued += length;
    handle_write(server, conn);

    if (conn->tx.head && conn->want_epollout == 0) {
        mod_epoll_event(server->epoll_fd, conn->fd,
            EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLOUT,
            fd_map_get(server->response, conn->fd));
        conn->want_epollout = 1;
    }
    return 0;
}

int validator_check_user(const server_t* server, const server_conn_t* conn) {
    if (int_map_exists(server->registered_clients, (int)conn->user_id) == -1) return -1;
    return 0;
}

uint32_t get_new_client_id(server_t* server) {
    uint32_t new_client_id = int_map_get_first_zero(server->registered_clients, (int)server->next_player_id);
    if (new_client_id == -1) new_client_id = server->next_player_id++;
    return new_client_id;
}

int unregister_client(server_t* server, server_conn_t* conn) {
    if (conn->session_id != 0) {
        session_bus_t* bus = fd_map_get(server->registered_sessions, (int)conn->session_id);
        session_message_t* system_message = malloc(sizeof(session_message_t));
        if (!system_message) return -1;
        system_message->type = SYSTEM_MESSAGE;
        system_message->data.system.type = CLIENT_UNREGISTER;
        system_message->data.system.id.client_id = conn->user_id;
        session_bus_push(&bus->read, system_message);
    }
    return int_map_remove(server->registered_clients, (int)conn->user_id);
}

int register_client(server_t* server,const uint32_t client_id, const int client_fd) {
    return int_map_set(server->registered_clients, (int)client_id, client_fd);
}