#include "Protocol/protocol.h"

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

int protocol_header_encode(header_t* header, const packet_type_t type, const uint16_t payload_length) {
    if (!header) return -1;
    if (payload_length + sizeof(header_t) > PROTOCOL_MAX_SIZE) return -1;

    header->magic = htons(PROTOCOL_MAGIC);
    header->version = PROTOCOL_VERSION;
    header->type = type;
    header->payload_length = htons(payload_length);
    return 0;
}

int protocol_header_decode(header_t* header_network, header_t* header_host) {
    if (!header_network || !header_host) return -1;

    header_host->magic = ntohs(header_network->magic);
    header_host->version = header_network->version;
    header_host->type = header_network->type;
    header_host->payload_length = ntohs(header_network->payload_length);
    return 0;
}

int protocol_header_validate(const header_t* header) {
    if (!header) return -1;

    if (header->magic != PROTOCOL_MAGIC) return -1;
    if (header->version != PROTOCOL_VERSION) return -1;
    if (header->payload_length + sizeof(header_t) > PROTOCOL_MAX_SIZE) return -1;
    return 0;
}

int try_peek_header(const uint8_t* buffer, const uint16_t buffer_length, header_t* header_out) {
    if (buffer_length < sizeof(header_t)) return -1;
    header_t header_network;
    memcpy(&header_network, buffer, sizeof(header_t));
    if (protocol_header_decode(&header_network, header_out) == -1) return -2;
    if (protocol_header_validate(header_out) == -1) return -2;
    return 0;
}