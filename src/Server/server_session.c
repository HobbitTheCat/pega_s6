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
    const server_session_t* session_info = fd_map_get(server->registered_sessions, (int)session_id);
    if (!session_info) return -1;
    session_bus_t* bus = session_info->bus;
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
    const server_session_t* session_info = fd_map_get(server->registered_sessions, (int)session_id);
    if (!session_info) return -1;
    session_bus_t* bus = session_info->bus;

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

int create_new_session(server_t* server, const uint8_t number_of_players, const uint8_t is_visible) {
    const uint32_t session_id = get_new_session_id(server);
    server_session_t* session_info = malloc(sizeof(server_session_t));
    if (!session_info) {printf("!session_info\n"); return -1;}
    session_t* new_session = (session_t*)malloc(sizeof(session_t));
    if (!new_session) {free(session_info); printf("!new_session\n"); return -1;}
    if (init_session(new_session, server->log_bus, session_id, number_of_players, is_visible) == -1) {free(new_session); free(session_info); printf("!init_session\n"); return -1;}
    response_t* response = create_response(P_BUS, new_session->bus->write.event_fd, &new_session->bus->write.queue);
    if (!response) {free(session_info); cleanup_session(new_session); free(new_session); printf("!response\n"); return -1;}
    if (add_epoll_event(server->epoll_fd, new_session->bus->write.event_fd, EPOLLIN, response) == -1) {free(response); free(session_info); cleanup_session(new_session); free(new_session); printf("!epoll_event\n"); return -1;}
    if (fd_map_set(server->response, new_session->bus->write.event_fd, response) == -1) {del_epoll_event(server->epoll_fd, new_session->bus->write.event_fd); free(response); free(session_info); cleanup_session(new_session); free(new_session); printf("!fd_map_set\n"); return -1;}

    session_info->is_visible = is_visible;
    session_info->capacity = number_of_players;
    session_info->number_players = 0;
    session_info->bus = new_session->bus;
    if (fd_map_set(server->registered_sessions, (int)session_id, session_info) == -1) {
        fd_map_remove(server->response, new_session->bus->write.event_fd);
        del_epoll_event(server->epoll_fd, new_session->bus->write.event_fd); free(response); free(session_info); cleanup_session(new_session); free(new_session); printf("!reg_map_set\n"); return -1;
    }

    pthread_t session;
    if (pthread_create(&session, NULL, session_main, new_session) != 0) {
        fd_map_remove(server->registered_sessions, (int)session_id);
        fd_map_remove(server->response, new_session->bus->write.event_fd);
        del_epoll_event(server->epoll_fd, new_session->bus->write.event_fd); free(response); free(session_info); cleanup_session(new_session); free(new_session);printf("!pthread_create\n");  return -1;
    }
    pthread_detach(session);
    return (int)session_id;
}

int unregister_session(const server_t* server, const uint32_t session_id) {
    server_session_t* session_info = fd_map_get(server->registered_sessions, (int)session_id);
    if (!session_info) return -1;
    const session_bus_t* bus = session_info->bus;
    del_epoll_event(server->epoll_fd, bus->write.event_fd);
    response_t* response = fd_map_get(server->response, bus->write.event_fd);
    if (response) {fd_map_remove(server->response, bus->write.event_fd); free(response);}
    fd_map_remove(server->registered_sessions, (int)session_id);
    free(session_info);
    send_system_message_to_session(server, session_id, 0, SESSION_UNREGISTERED);
    return 0;
}

int get_available_sessions(const server_t* server, uint32_t* session_list) {
    int list_index = 0;
    for (int i = 1; i < server->next_session_id; i++) {
        server_session_t* session_info = fd_map_get(server->registered_sessions, i);
        if (!session_info) continue;
        if (session_info->is_visible == 1 && session_info->capacity - session_info->number_players > 0) {
            session_list[list_index] = i;
            list_index ++;
        }
    }
    return list_index;
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
            // printf("Received return message type\n");
            // print_session_message(message);
            if (message->type == USER_MESSAGE) {
                const server_player_t* player = fd_map_get(server->registered_players, (int)message->data.user.client_id);
                if (player && player->conn) send_packet(server, player->conn, message->data.user.packet_type, message->data.user.buf, message->data.user.payload_length);
            } else {
                const system_message_type_t msg_type = message->data.system.type;
                if (msg_type == SESSION_UNREGISTER) {
                    printf("Got unreg for %d\n", message->data.system.session_id);
                    unregister_session(server,  message->data.system.session_id);
                }
                else {
                    server_player_t* player = fd_map_get(server->registered_players, (int)message->data.system.user_id);
                    if (!player) break;
                    server_session_t* session_info = fd_map_get(server->registered_sessions, (int)message->data.system.session_id);
                    if (msg_type == USER_CONNECTED) {
                        session_info->number_players += 1;
                        player->session_id = message->data.system.session_id;
                        player->state = PLAYER_STATE_IN_GAME;
                    } else if (msg_type == USER_DISCONNECTED) {
                        session_info->number_players -= 1;
                        player->session_id = 0;
                        player->state = PLAYER_STATE_LOBBY;
                    } else if (msg_type == UPDATE_STATE) {
                        session_info->number_players = message->data.system.player_count;
                        session_info->is_visible = message->data.system.is_visible;
                        session_info->capacity = message->data.system.capacity;
                    } else {
                        printf("Unknown message type %d\n", message->data.system.type);
                        print_session_message(message);
                    }
                }
            }
            free(message);
        }
    }
}