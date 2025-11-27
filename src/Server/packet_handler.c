#include <string.h>
#include <stdio.h>
#include "Server/server.h"

int handle_packet_main(server_t* server, frame_t* frame) {
    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    int packet_size = 0;
    if (!buffer) return -1;
    printf("Packet type %d \n", frame->packet_type);
    switch (frame->packet_type) {
        case PKT_PING:
            packet_size = create_simple_packet(buffer, PKT_PONG, frame->session_id, frame->user_id);
            break;
        case PKT_HELLO:
            packet_size = create_simple_packet(buffer, PKT_WELCOME, frame->session_id, frame->user_id);
            break;
        case PKT_REGISTER:
            if (validator_check_user(server, frame) == 1) {
                packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x02, "User already registered");
                break;
            }
            const uint32_t user_id = get_new_client_id(server);
            register_client(server, user_id, frame->fd);
            packet_size = create_simple_packet(buffer, PKT_SYNC_STATE, frame->session_id, user_id);
            break;
        case PKT_UNREGISTER:
            if (validator_check_user(server, frame) == -1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x03, "User is not registered");break;}
            if (int_map_get(server->registered_clients, (int)frame->user_id) != frame->fd) {
                packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x04, "Unauthorized access");
                break;
            }
            // удалить из registered_clients
            // TODO отправить служебное сообщение в сессию (если есть)
            break;
        case PKT_WELCOME:
        case PKT_PONG:
            packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x01, "Packet not authorized");
            break;
        case PKT_SESSION_CREATE:
            if (validator_check_user(server, frame) == -1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x03, "User is not registered"); printf("User is not registered");break;}
            if (validator_check_session(server, frame) == 1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x04, "Session already exist"); printf("Session already exist");break;}
            // TODO добавить более сложные пакеты для создания сессий
            const uint32_t session_id = create_new_session(server, 4);
            packet_size = create_simple_packet(buffer, PKT_SESSION_END, session_id, frame->user_id);
            break;
        default:
            if (validator_check_user(server, frame) == -1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x03, "User is not registered");break;}
            if (validator_check_session(server, frame) == 1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x05, "Session not found");break;}
            session_message_t* message = malloc(sizeof(session_message_t));
            if (!message) {free(buffer); return -1;}
            message->client_id = frame->user_id;
            message->session_id = frame->session_id;
            message->client_fd = frame->fd;
            message->packet_type = frame->packet_type;
            memcpy(message->buf, frame->rx.buf, frame->rx.need);
            printf("Default \n");
            const fd_map_t* registered_sessions = server->registered_sessions;
            session_bus_t* bus = fd_map_get(registered_sessions, (int)frame->session_id);
            if (session_bus_push(&bus->read, message) != BUFFER_SUCCESS) {
                free(message);
                free(buffer);
                return -1;
            }
            printf("Default \n");
            free(buffer);
            return 0;
    }

    enqueue_message(server, frame, buffer, packet_size);
    free(buffer);
    return 0;
}
