#include <stdio.h>
#include <stdlib.h>

#include "Server/server.h"
#include "Protocol/protocol.h"

int handle_packet_main(server_t* server, server_conn_t* conn) {
    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;
    int packet_size = 0;
    switch (conn->packet_type) {
        case PKT_PING:
            packet_size = create_simple_packet(buffer, PKT_PONG, conn->session_id, conn->user_id);
            break;
        case PKT_HELLO:
            packet_size = create_simple_packet(buffer, PKT_WELCOME, conn->session_id, conn->user_id);
            break;
        case PKT_REGISTER:
            if (validator_check_user(server, conn) == 1) {
                packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x02, "User already registered");
                break;
            }
            const uint32_t user_id = get_new_client_id(server);
            register_client(server, user_id, conn->fd);
            packet_size = create_simple_packet(buffer, PKT_SYNC_STATE, conn->session_id, user_id);
            break;
        case PKT_UNREGISTER:
            if (validator_check_user(server, conn) == -1) {packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x03, "User is not registered");break;}
            if (int_map_get(server->registered_clients, (int)conn->user_id) != conn->fd) {
                packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x04, "Unauthorized access");
                break;
            }
            unregister_client(server, conn);
            break;
        case PKT_WELCOME:
        case PKT_PONG:
            packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x01, "Packet not authorized");
            break;
        case PKT_SESSION_CREATE:
            if (validator_check_user(server, conn) == -1) {packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x03, "User is not registered"); printf("User is not registered");break;}
            if (validator_check_session(server, conn) == 0) {packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x04, "Session already exist"); printf("Session already exist");break;}
            // TODO добавить более сложные пакеты для создания сессий
            const uint32_t session_id = create_new_session(server, 4);
            session_bus_t* session_bus = fd_map_get(server->registered_sessions, (int)session_id);
            send_user_message(&session_bus->read, conn->user_id, conn->fd, session_id, PKT_SESSION_JOIN, sizeof(header_t), conn->rx.buf);
            return 0;
        default:
            if (validator_check_user(server, conn) == -1) {packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x03, "User is not registered");break;}
            if (validator_check_session(server, conn) == -1) {packet_size = create_error_packet(buffer, conn->session_id, conn->user_id, 0x05, "Session not found");break;}

            printf("Default \n");
            const fd_map_t* registered_sessions = server->registered_sessions;
            session_bus_t* bus = fd_map_get(registered_sessions, (int)conn->session_id);

            send_user_message(&bus->read, conn->user_id, conn->fd, conn->session_id,
                conn->packet_type, conn->rx.need, conn->rx.buf);
            free(buffer);
            return 0;
    }

    enqueue_message(server, conn, buffer, packet_size);
    free(buffer);
    return 0;
}
