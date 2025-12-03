#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC 0x5053u // 'PS'
#define PROTOCOL_VERSION 1
#define PROTOCOL_MAX_SIZE 4090

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
    uint16_t payload_length;    // payload
} header_t; // 6 bytes
#pragma pack(pop)

_Static_assert(sizeof(header_t) == 6, "header must be 6 bytes");

int protocol_header_encode(header_t* header, packet_type_t type, uint16_t payload_length);
int protocol_header_decode(header_t* header_network, header_t* header_host);

int protocol_header_validate(const header_t* header);
int try_peek_header(const uint8_t* buffer, uint16_t buffer_length, header_t* header_out);

#pragma pack(push, 1)
typedef struct {
    uint8_t  error_code;
    uint16_t message_length;
} pkt_error_payload_t;
#pragma pack(pop)

#endif //PROTOCOL_H
