#include "Server/server.h"
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
            if (message->type == USER_MESSAGE) {
                const response_t* client_response = fd_map_get(server->response, (int)message->data.user.client_fd);
                if (client_response->type != P_CLIENT) {printf("NO COMMENTS client fd is not client?\n");free(message);return;}
                enqueue_message(server, client_response->ptr, message->data.user.buf, message->data.user.message_length);
            } else {
                switch (message->data.system.type) {
                    case SESSION_UNREGISTER:
                        session_t* session = fd_map_get(server->registered_sessions, (int)message->data.system.id.session_id);
                        unregister_session(server, session);
                        break;
                    default:
                        printf("Unknown message type %d\n", message->data.system.type);
                        print_session_message(message);
                        break;
                }
            }
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

    response_t* response = create_response(P_BUS, new_session->bus->write.event_fd, &new_session->bus->write.queue);
    if (!response) {cleanup_session(new_session); return -1;}
    if (add_epoll_event(server->epoll_fd, new_session->bus->write.event_fd, EPOLLIN, response) == -1) {
        cleanup_session(new_session); free(response); return -1;
    }
    fd_map_set(server->response, new_session->bus->write.event_fd, response);

    pthread_t session;
    if (pthread_create(&session, NULL, session_main, new_session) != 0) {
        fd_map_remove(server->registered_sessions, (int)session_id);
        cleanup_session(new_session);
        return -1;
    }
    pthread_detach(session);
    return (int)session_id;
}

int unregister_session(const server_t* server, session_t* session) {
    del_epoll_event(server->epoll_fd, session->bus->write.event_fd);

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