#include <stdio.h>

#include "Server/server.h"
#include "Protocol/protocol.h"


void handle_packet_main(server_t* server, server_conn_t* conn, const uint8_t packet_type, const uint8_t* payload, const uint16_t payload_length) {
    if (packet_type == PKT_PING) { send_simple_packet(server, conn, PKT_PONG); return;}
    if (packet_type == PKT_HELLO) {conn->state = CONN_STATE_HANDSHAKE; send_simple_packet(server, conn, PKT_WELCOME); return;}
    if (conn->state == CONN_STATE_INIT) {send_error_packet(server, conn, 0x01, "Wrong packet order"); return;}
    if (packet_type == PKT_REGISTER) {
        if (conn->player != NULL) { send_error_packet(server, conn, 0x02, "User already registered"); return;}
        register_client(server, conn);
        send_sync_state(server, conn);
        return;
    }
    if (packet_type == PKT_RECONNECT) {
        handle_reconnect_packet(server, conn, payload, payload_length); return;
    }
    if (conn->player == NULL) { send_error_packet(server, conn, 0x03, "User is not registered"); return;}
    const server_player_t* player = conn->player;
    switch (packet_type) {
        case PKT_GET_SESSION_LIST:
            send_session_list(server, conn); break;
        case PKT_UNREGISTER:
            unregister_client(server, player->user_id);
            send_simple_packet(server, conn, PKT_UNREGISTER_RETURN); break;
        case PKT_SESSION_CREATE:
            if (player->session_id != 0) { send_error_packet(server, conn, 0x04, "Session already exist"); break;}
            const uint32_t session_id = create_new_session(server, 4, 1);
            send_message_to_session(server, session_id, player->user_id, PKT_SESSION_JOIN, payload, payload_length);
            break;
        case PKT_SESSION_JOIN:
            const uint32_t id = handle_session_join_packet(server, conn, payload, payload_length);
            if (id == 0) break;
            send_message_to_session(server, id, player->user_id, PKT_SESSION_JOIN, payload, payload_length);
            break;
        default:
            if (player->session_id == 0) { send_error_packet(server, conn, 0x05, "Session not found"); break;}
            send_message_to_session(server, player->session_id, player->user_id, packet_type, payload, payload_length);
    }
}