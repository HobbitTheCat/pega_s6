#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "Session/session.h"

int session_send_to_player(const session_t* session, const uint32_t user_id, const uint8_t packet_type, const void* payload, const uint16_t payload_length) {
    if (payload_length > SP_MAX_FRAME) return -1;

    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    if (!message) return -1;
    message->type = USER_MESSAGE;
    message->data.user.client_id = user_id;
    message->data.user.packet_type = packet_type;
    message->data.user.payload_length = payload_length;
    if (payload_length > 0 && payload) memcpy(message->data.user.buf, payload, payload_length);
    if (session_bus_push(&session->bus->write, message) != 0) {free(message);return -1;}
    return 0;
}

int send_system_message_to_server(const session_t* session, const uint32_t user_id, const system_message_type_t type) {
    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    message->type = SYSTEM_MESSAGE;
    message->data.system.type = type;
    message->data.system.session_id = session->id;
    message->data.system.user_id = user_id;
    if (session_bus_push(&session->bus->write, message) != 0) {free(message);return -1;}
    return 0;
}

int session_send_simple_packet(const session_t* session, const uint32_t user_id, const uint8_t packet_type) {
    return session_send_to_player(session, user_id, packet_type, NULL, 0);
}

int session_send_error_packet(const session_t* session, const uint32_t user_id, const uint8_t error_code, const char* error_message) {
    if (!error_message) error_message = "";
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
    const int result = session_send_to_player(session, user_id, PKT_ERROR, payload, payload_length);
    free(payload);
    return result;
}