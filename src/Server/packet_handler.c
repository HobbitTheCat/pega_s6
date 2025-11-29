#include <string.h>
#include <stdio.h>
#include "Server/server.h"

int handle_packet_main(server_t* server, frame_t* frame) {
    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;
    int packet_size = 0;
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
            session_bus_t* session_bus = fd_map_get(server->registered_sessions, (int)session_id);
            send_user_message(&session_bus->read, frame->user_id, frame->fd, session_id, PKT_SESSION_JOIN, sizeof(header_t), frame->rx.buf); //TODO не уверен что работает
            return 0;
        default:
            if (validator_check_user(server, frame) == -1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x03, "User is not registered");break;}
            if (validator_check_session(server, frame) == 1) {packet_size = create_error_packet(buffer, frame->session_id, frame->user_id, 0x05, "Session not found");break;}

            printf("Default \n");
            const fd_map_t* registered_sessions = server->registered_sessions;
            session_bus_t* bus = fd_map_get(registered_sessions, (int)frame->session_id);

            send_user_message(&bus->read, frame->user_id, frame->fd, frame->session_id,
                frame->packet_type, frame->rx.need, frame->rx.buf);
            free(buffer);
            return 0;
    }

    enqueue_message(server, frame, buffer, packet_size);
    free(buffer);
    return 0;
}
