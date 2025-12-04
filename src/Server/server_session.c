#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/epoll.h>

#include "Server/server.h"
#include "Session/session.h"

int send_message_to_session(const server_t* server, const uint32_t session_id, const uint32_t user_id, const uint8_t packet_type, const uint8_t* payload, const uint16_t payload_length) {
    session_bus_t* bus = fd_map_get(server->registered_sessions, (int)session_id);
    if (!bus) return -1;
    if (payload_length > SP_MAX_FRAME) return -1;

    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    message->type = USER_MESSAGE;
    message->data.user.client_id = user_id;
    message->data.user.packet_type = packet_type;
    message->data.user.payload_length = payload_length;
    memcpy(message->data.user.buf, payload, payload_length);
    if (session_bus_push(&bus->read, message) != 0) {free(message); return -1;}
    return 0;
}

int send_system_message_to_session(const server_t* server, const uint32_t session_id, const uint32_t user_id, const system_message_type_t type) {
    session_bus_t* bus = fd_map_get(server->registered_sessions, (int)session_id);
    if (!bus) return -1;

    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    message->type = SYSTEM_MESSAGE;
    message->data.system.type = type;
    message->data.system.session_id = session_id;
    message->data.system.user_id = user_id;
    if (session_bus_push(&bus->read, message) != 0) {free(message); return -1;}
    return 0;
}

uint32_t get_new_session_id(server_t* server) {
    int new_session_id = fd_map_get_first_null_id(server->registered_sessions, (int)server->next_session_id);
    if (new_session_id == -1) new_session_id = (int)server->next_session_id++;
    return new_session_id;
}

int create_new_session(server_t* server, const uint8_t number_of_players) {
    const uint32_t session_id = get_new_session_id(server);
    session_t* new_session = (session_t*)malloc(sizeof(session_t));
    if (!new_session) return -1;
    if (init_session(new_session, session_id, number_of_players) == -1) {free(new_session); return -1;}
    fd_map_set(server->registered_sessions, (int)session_id, new_session->bus);

    response_t* response = create_response(P_BUS, new_session->bus->write.event_fd, &new_session->bus->write.queue);
    if (!response) {cleanup_session(new_session); return -1;}
    if (add_epoll_event(server->epoll_fd, new_session->bus->write.event_fd, EPOLLIN, response) == -1) {
        free(response); cleanup_session(new_session); return -1;
    }
    fd_map_set(server->response, new_session->bus->write.event_fd, response); //TODO я все понял я могу очищать сессии через это так что будь добр пихни это в cleanup_server

    pthread_t session;
    if (pthread_create(&session, NULL, session_main, new_session) != 0) {
        fd_map_remove(server->registered_sessions, (int)session_id);
        cleanup_session(new_session); return -1;
    }
    pthread_detach(session);
    return (int)session_id;
}

int unregister_session(const server_t* server, const uint32_t session_id) {
    const session_bus_t* bus = fd_map_get(server->registered_sessions, (int)session_id);
    del_epoll_event(server->epoll_fd, bus->write.event_fd);
    fd_map_remove(server->registered_sessions, (int)session_id);
    send_system_message_to_session(server, session_id, 0, SESSION_UNREGISTERED);
    return 0;
}

void handle_bus_message(server_t* server, const response_t* response) {
    uint64_t count = 0;
    for (;;) {
        const ssize_t rd = read(response->fd, &count, sizeof(uint64_t));
        if (rd == sizeof(count)) break;
        if (rd == -1 && errno == EAGAIN) break;
        if (rd == -1 && errno == EINTR) continue;
        break;
    }

    while (count--) {
        session_message_t* message = NULL;
        const int received = buffer_pop(response->ptr, &message);
        if (received == BUFFER_EEMPTY) break;
        if (received == BUFFER_SUCCESS && message) {
            printf("Received return message type\n");
            print_session_message(message);
            if (message->type == USER_MESSAGE) {
                const server_player_t* player = fd_map_get(server->registered_players, (int)message->data.user.client_id);
                if (player && player->conn) send_packet(server, player->conn, message->data.user.packet_type, message->data.user.buf, message->data.user.payload_length);
            } else {
                switch (message->data.system.type) {
                    case SESSION_UNREGISTER:
                        unregister_session(server,  message->data.system.session_id);
                        break;
                    case USER_CONNECTED:
                        server_player_t* conn = fd_map_get(server->registered_players, (int)message->data.system.user_id);
                        conn->session_id = message->data.system.session_id;
                        conn->state = PLAYER_STATE_IN_GAME;
                        break;
                    case USER_DISCONNECTED:
                        server_player_t* disconn = fd_map_get(server->registered_players, (int)message->data.system.user_id);
                        disconn->session_id = 0;
                        disconn->state = PLAYER_STATE_LOBBY;
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