#include "Server/server.h"
#include "Server/frame.h"
#include "SupportStructure/fd_map.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/epoll.h>

int enqueue_message(const server_t* server, frame_t* frame, const uint8_t* buf, const uint32_t length) {
    if (frame->tx.queued + length > frame->tx.high_watermark) return -1;

    out_chunk_t* chunk = malloc(sizeof(*chunk));
    if (!chunk) return -1;
    chunk->data = malloc(length);
    if (!chunk->data) {free(chunk); return -1;}
    memcpy(chunk->data, buf, length);
    chunk->len = length;
    chunk->off = 0;
    chunk->next = NULL;

    if (!frame->tx.tail) frame->tx.head = frame->tx.tail = chunk;
    else {frame->tx.tail->next = chunk; frame->tx.tail = chunk;}

    frame->tx.queued += length;
    handle_write(server, frame);

    if (frame->tx.head && frame->tx.epollout == 0) {
        mod_epoll_event(server->epoll_fd, frame->fd,
            EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLOUT,
            fd_map_get(server->clients, frame->fd));
        frame->tx.epollout = 1;
    }
    return 0;
}

int validator_check_user(const server_t* server, const frame_t* frame) {
    if (int_map_exists(server->registered_clients, (int)frame->user_id) == -1) return -1;
    return 0;
}

uint32_t get_new_client_id(server_t* server) {
    uint32_t new_client_id = int_map_get_first_zero(server->registered_clients, (int)server->next_player_id);
    if (new_client_id == -1) new_client_id = server->next_player_id++;
    return new_client_id;
}

int unregister_client(server_t* server,const uint32_t client_id) { //TODO добавить отправку сообщения в сессию
    return int_map_remove(server->registered_clients, (int)client_id);
}

int register_client(server_t* server,const uint32_t client_id, const int client_fd) {
    return int_map_set(server->registered_clients, (int)client_id, client_fd);
}