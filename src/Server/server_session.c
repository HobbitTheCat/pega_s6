#include "Server/server.h"
#include "SupportStructure/session_bus.h"
#include "SupportStructure/fd_map.h"
#include "Session/session.h"

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/epoll.h>

void handle_bus_message(const server_t* server, const response_t* response) {
    uint64_t count = 0;
    for (;;) {
        const ssize_t rd = read(response->fd, &count, sizeof(count));
        if (rd == sizeof(count)) break;
        if (rd == -1 && errno == EAGAIN) break;
        if (rd == -1 && errno == EINTR) continue;
        break;
    }

    while (count --) {
        session_message_t* message = NULL;
        const int received = buffer_pop(response->ptr, &message);
        if (received == BUFFER_EEMPTY) break;
        if (received == BUFFER_SUCCESS && message) {
            const response_t* client_response = fd_map_get(server->clients, (int)message->client_fd);
            if (client_response->type != P_CLIENT) {printf("NO COMMENTS client fd is not client?");free(message);return;}
            enqueue_message(server, client_response->ptr, message->buf, message->message_length);
            free(message);
        }
    }
}

int create_new_session(server_t* server, const uint8_t number_of_players) {
    const uint32_t session_id = get_new_session_id(server);
    session_t* new_session = malloc(sizeof(session_t));
    if (!new_session) return -1;
    if (init_session(new_session, session_id, number_of_players) == -1) {free(new_session); return -1;}
    fd_map_set(server->registered_sessions, (int)session_id, new_session->bus);
    pthread_t session;
    if (pthread_create(&session, NULL, session_main, new_session) != 0) {
        fd_map_remove(server->registered_sessions, (int)session_id);
        cleanup_session(new_session);
        return -1;
    }
    pthread_detach(session);
    return (int)session_id;
}

int unregister_session(const server_t* server, session_t* session) { //TODO переписать что бы только удалять записи из реестра все остальное в самой сессии
    fd_map_remove(server->registered_sessions, (int)session->id);
    session->running = 0;
    const uint64_t one = 1; write(session->bus->write.event_fd, &one, sizeof(one));
    return 0;
}

int validator_check_session(const server_t* server, const frame_t* frame) {
    if (fd_map_exists(server->registered_sessions, (int)frame->session_id) == -1)
        return -1;
    return 0;
}

uint32_t get_new_session_id(server_t* server) {
    uint32_t new_session_id = fd_map_get_first_null_id(server->registered_sessions, (int)server->next_session_id);
    if (new_session_id == -1) new_session_id = server->next_session_id++;
    return new_session_id;
}