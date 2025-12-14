#include <stdio.h>

#include "Server/server.h"
#include "Protocol/protocol.h"


int handle_packet_main(server_t* server, server_conn_t* conn, const uint8_t packet_type, const uint8_t* payload, const uint16_t payload_length) {
    if (packet_type == PKT_PING) { return send_simple_packet(server, conn, PKT_PONG);}
    if (packet_type == PKT_HELLO) {conn->state = CONN_STATE_HANDSHAKE; return send_simple_packet(server, conn, PKT_WELCOME);}
    if (conn->state == CONN_STATE_INIT) {return send_error_packet(server, conn, 0x01, "Wrong packet order");}
    if (packet_type == PKT_REGISTER) {
        if (conn->player != NULL) { return send_error_packet(server, conn, 0x02, "User already registered");}
        register_client(server, conn);
        return send_sync_state(server, conn);
    }
    if (packet_type == PKT_RECONNECT) {
        return handle_reconnect_packet(server, conn, payload, payload_length);
    }
    if (conn->player == NULL) { return send_error_packet(server, conn, 0x03, "User is not registered");}
    const server_player_t* player = conn->player;
    int result;
    uint32_t id;
    switch (packet_type) {
        case PKT_GET_SESSION_LIST:
            return send_session_list(server, conn);
        case PKT_UNREGISTER:
            result = send_simple_packet(server, conn, PKT_UNREGISTER_RETURN);
            unregister_client(server, player->user_id);
            return result;
        case PKT_SESSION_CREATE:
            if (player->session_id != 0) { send_error_packet(server, conn, 0x04, "Session already exist"); break;}
            const int session_id = handle_session_create_packet(server, conn, payload, payload_length);
            if (session_id < 0) send_error_packet(server, conn, 0x05, "Session creation failed");
            send_message_to_session(server, session_id, player->user_id, PKT_SESSION_JOIN, payload, payload_length);
            break;
        case PKT_SESSION_JOIN:
            id = handle_session_join_packet(server, conn, payload, payload_length);
            if (id == 0) break;
            if (!fd_map_get(server->registered_sessions, (int)id)) { send_error_packet(server, conn, 0x05, "Session not found"); break;}
            send_message_to_session(server, id, player->user_id, PKT_SESSION_JOIN, payload, payload_length);
            break;
        default:
            if (player->session_id == 0) { send_error_packet(server, conn, 0x05, "Session not found"); break;}
            send_message_to_session(server, player->session_id, player->user_id, packet_type, payload, payload_length);
    }
    return 0;
}