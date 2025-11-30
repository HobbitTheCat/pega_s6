#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define PROTOCOL_MAGIC 0x5053u // 'PS'
#define PROTOCOL_VERSION 1
#define PROTOCOL_MAX_SIZE 4096

typedef enum {
    PKT_PING = 1,
    PKT_PONG = 2,
    PKT_HELLO = 3,
    PKT_WELCOME = 4,
    PKT_REGISTER = 5,
    PKT_UNREGISTER = 6,
    PKT_SYNC_STATE = 7,

    PKT_SESSION_JOIN = 8,
    PKT_SESSION_LEAVE = 9,
    PKT_SESSION_CREATE = 10,
    PKT_SESSION_CLOSE = 11,
    PKT_SESSION_STATE = 12,        // <-

    PKT_READY = 13,             // ->
    PKT_START_SESSION = 14,        // ->
    PKT_START_SESSION_RETURN = 15, // <-
    PKT_INFO = 16,
    PKT_REQUEST_EXTRA = 17,
    PKT_RESPONSE_EXTRA = 18,
    PKT_PHASE_RESULT = 19,
    PKT_SESSION_END = 20,
    // ...
    PKT_ERROR = 255
} packet_type_t;

#pragma pack(push, 1)
typedef struct {
    uint16_t magic;     // PROTOCOL_MAGIC
    uint8_t version;    // PROTOCOL_VERSION
    uint8_t type;       // packet_type_t
    uint16_t length;    // header + payload
    uint32_t user_id;
    uint32_t session_id;
} header_t; // 14 bytes
#pragma pack(pop)

_Static_assert(sizeof(header_t) == 14, "header must be 14 bytes");

int protocol_header_encode(header_t* h, packet_type_t type, uint32_t user_id, uint32_t session_id, uint16_t total_len);

int protocol_header_decode(const header_t* h_net, header_t* h_host);

int protocol_header_validate(const header_t* h);

int try_peek_header(const uint8_t* buf, uint32_t have, header_t* h_out);

void print_header(const header_t* h);


int create_simple_packet(uint8_t* packet, packet_type_t packet_type, uint32_t session_id, uint32_t user_id);
int create_error_packet(uint8_t* packet, uint32_t session_id, uint32_t user_id, uint8_t error_code, char* error_message);
#endif //PROTOCOL_H
