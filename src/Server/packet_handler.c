#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server_conn.h"
#include "Server/server.h"
#include "Protocol/protocol.h"
#include "SupportStructure/session_bus.h"



void handle_packet_main(server_t* server, server_conn_t* conn, const uint8_t packet_type, const uint8_t* payload, const uint16_t payload_length) {
    switch (packet_type) {
        case PKT_PING: send_simple_packet(server, conn, PKT_PONG); break;
        case PKT_HELLO:
            conn->state = CONN_STATE_HANDSHAKE;
            send_simple_packet(server, conn, PKT_WELCOME); break;
        case PKT_REGISTER:
            if (conn->user_id != 0) { send_error_packet(server, conn, 0x02, "User already registered"); break;}
            register_client(server, conn);
            send_simple_packet(server, conn, PKT_SYNC_STATE); break;
        case PKT_UNREGISTER:
            if (conn->user_id == 0) { send_error_packet(server, conn, 0x03, "User is not registered"); break;}
            unregister_client(server, conn);
            send_simple_packet(server, conn, PKT_SYNC_STATE); break;
        case PKT_SESSION_CREATE:
            if (conn->user_id == 0) { send_error_packet(server, conn, 0x03, "User is not registered"); break;}
            if (conn->session_id != 0) { send_error_packet(server, conn, 0x04, "Session already exist"); break;}
            const uint32_t session_id = create_new_session(server, 4);
            send_message_to_session(server, session_id, conn->user_id, PKT_SESSION_JOIN, payload, payload_length);
            break;
        default:
            if (conn->user_id == 0) { send_error_packet(server, conn, 0x03, "User is not registered"); break;}
            if (conn->session_id == 0) { send_error_packet(server, conn, 0x05, "Session not found"); break;}
            send_message_to_session(server, conn->session_id, conn->user_id, packet_type, payload, payload_length);
    }
}