#include "Server/server.h"
#include "Protocol/proto_io.h"
#include "SupportStructure/fd_map.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>

int enqueue_message(server_t* server, server_conn_t* conn, const uint8_t* buffer, const uint32_t total_length) {
    if (conn->tx.queued + total_length > conn->tx.high_watermark) return -1;

    out_chunk_t* chunk = malloc(sizeof(*chunk));
    if (!chunk) return -1;

    chunk->data = malloc(total_length);
    if (!chunk->data) {free(chunk); return -1;}
    memcpy(chunk->data, buffer, total_length);
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
    const int result = enqueue_message(server, conn, buff, total_length);
    free(buff);
    return result;
}

uint32_t get_new_client_id(server_t* server) {
    uint32_t new_client_id = fd_map_get_first_null_id(server->registered_clients, (int)server->next_player_id);
    if (new_client_id == -1) new_client_id = server->next_player_id++;
    return new_client_id;
}

int register_client(server_t* server, server_conn_t* conn) {
    const uint32_t new_client_id = get_new_client_id(server);
    if (fd_map_set(server->registered_clients, (int)new_client_id, conn) == -1) return -1;
    conn->state = CONN_STATE_LOBBY;
    conn->user_id = new_client_id;
    return 0;
}

int unregister_client(server_t* server, server_conn_t* conn) {
    if (conn->session_id != 0) send_system_message_to_session(server, conn->session_id, conn->user_id, USER_UNREGISTER);
    fd_map_remove(server->registered_clients, (int)conn->user_id);
    conn->state = CONN_STATE_HANDSHAKE;
    return 0;
}
