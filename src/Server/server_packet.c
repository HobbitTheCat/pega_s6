#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "protocol.h"
// #include "server_conn.h"
#include "Server/server.h"

int send_simple_packet(server_t* server, server_conn_t* conn, const uint8_t type) {
    return send_packet(server, conn, type, NULL, 0);
}

int send_error_packet(server_t* server, server_conn_t* conn, const uint8_t error_code, const char* error_message) {
    if (!error_message) error_message = "";
    printf("Server error (%u): %s\n", error_code, error_message);
    const size_t message_length = strlen(error_message);
    const size_t header_part = sizeof(pkt_error_payload_t);
    const size_t payload_length = header_part + message_length;

    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_error_payload_t* packet = (pkt_error_payload_t*)payload;
    packet->error_code = error_code;
    packet->message_length = (uint16_t)message_length;
    if (message_length > 0) memcpy((uint8_t*)(packet + 1), error_message, message_length);
    const int result = send_packet(server, conn, PKT_ERROR, payload, payload_length);
    free(payload);
    return result;
}

int send_sync_state(server_t* server, server_conn_t* conn) {
    if (!conn->player) return -1;

    const server_player_t* player = conn->player;
    const char* token = player->reconnection_token;
    if (!token) token = "";
    const size_t token_length = strlen(token);
    if (token_length > PROTOCOL_MAX_SIZE) return -1;

    const size_t header_part = sizeof(pkt_sync_state_payload_t);
    const size_t payload_length = header_part + token_length;

    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_sync_state_payload_t* pkt = (pkt_sync_state_payload_t*)payload;
    pkt->user_id = player->user_id;
    pkt->token_length = token_length;

    if (token_length > 0) memcpy((uint8_t*)(pkt + 1), token, token_length);
    const int result = send_packet(server, conn, PKT_SYNC_STATE, payload, payload_length);
    free(payload);
    return result;
}

int handle_reconnect_packet(server_t* server, server_conn_t* conn, const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_reconnect_payload_t)) {
        send_error_packet(server, conn, 0x21, "Bad reconnect length");
        return -1;
    }

    const pkt_reconnect_payload_t* pkt = (const pkt_reconnect_payload_t*)payload;
    const uint32_t user_id   = pkt->user_id;
    const uint16_t token_len = pkt->token_length;
    const size_t header_part = sizeof(*pkt);

    if (payload_length < header_part + token_len) {
        send_error_packet(server, conn, 0x22, "Truncated reconnect token");
        return -1;
    }

    const uint8_t* token_ptr = (const uint8_t*)(pkt + 1);
    char token_buf[256];
    if (token_len >= sizeof(token_buf)) {
        send_error_packet(server, conn, 0x23, "Reconnect token too long");
        return -1;
    }
    memcpy(token_buf, token_ptr, token_len);
    token_buf[token_len] = '\0';
    const int rc = reconnect_client(server, conn, user_id, token_buf);
    if (rc == -1) {
        send_error_packet(server, conn, 0x24, "Player not found");
        return -1;
    }
    if (rc == -2) {
        send_error_packet(server, conn, 0x25, "Invalid token");
        return -1;
    }
    send_sync_state(server, conn);
    return 0;
}