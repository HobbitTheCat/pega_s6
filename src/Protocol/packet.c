#include <string.h>

#include "bytebuffer.h"
#include "Protocol/protocol.h"

int create_simple_packet(uint8_t* packet, const packet_type_t packet_type, const uint32_t session_id, const uint32_t user_id) {
    header_t header;
    const int header_size = sizeof(header_t);
    protocol_header_encode(&header, packet_type, user_id, session_id,header_size);
    memcpy(packet, &header, header_size);
    return header_size;
}

int create_error_packet(uint8_t* packet, const uint32_t session_id, const uint32_t user_id, const uint8_t error_code, char* error_message) {
    bytebuffer_t* buffer = bytebuffer_wrap(packet, PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;

    bytebuffer_set_cursor(buffer, sizeof(header_t));
    bytebuffer_put_uint8(buffer, error_code);
    bytebuffer_put_uint32(buffer, strlen(error_message));
    bytebuffer_put_bytes(buffer, (uint8_t*)error_message, strlen(error_message));
    const uint32_t payload_size = buffer->position;

    header_t header;
    protocol_header_encode(&header, PKT_ERROR, user_id, session_id, payload_size);
    memcpy(packet, &header, sizeof(header_t));

    bytebuffer_clean(buffer);
    return (int)payload_size;
}