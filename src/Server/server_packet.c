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
    printf(error_message);
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