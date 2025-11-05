#include "protocol.h"

#include <string.h>
#include <netinet/in.h>

int protocol_header_encode(header_t* h, packet_type_t type, uint32_t user_id, uint32_t session_id, uint16_t total_len) {
    if (!h) return -1;
    if (total_len < sizeof(header_t) || total_len > PROTOCOL_MAX_SIZE) return -1;

    h->magic = htons((uint16_t)PROTOCOL_MAGIC);
    h->version = PROTOCOL_VERSION;
    h->type = (uint8_t)type;
    h->length = htons(total_len);
    h->user_id = htonl(user_id);
    h->session_id = htonl(session_id);
    return 0;
}

int protocol_header_decode(const header_t* h_net, header_t* h_host) {
    if (!h_net || !h_host) return -1;

    h_host->magic = ntohs(h_net->magic);
    h_host->version = h_net->version;
    h_host->type = h_net->type;
    h_host->length = ntohs(h_net->length);
    h_host->user_id = ntohl(h_net->user_id);
    h_host->session_id = ntohl(h_net->session_id);

    return 0;
}

int protocol_header_validate(const header_t* h) {
    if (!h) return -1;

    if (h->magic != PROTOCOL_MAGIC) return -1;
    if (h->version != PROTOCOL_VERSION) return -1;
    if (h->length < sizeof(header_t) || h->length > PROTOCOL_MAX_SIZE) return -1;
    return 0;
}

int try_peek_header(const uint8_t* buf, uint32_t have, header_t* h_out) {
    if (have < sizeof(header_t)) return -1;
    header_t net;
    memcpy(&net, buf, sizeof(net));
    if (protocol_header_decode(&net, h_out) == -1) return -2;
    if (protocol_header_validate(h_out) == -1) return -2;
    return 0;
}


