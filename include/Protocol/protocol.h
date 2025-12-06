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
    PKT_RECONNECT = 6,
    PKT_UNREGISTER = 7,
    PKT_UNREGISTER_RETURN = 8,
    PKT_SYNC_STATE = 9,

    PKT_GET_SESSION_LIST = 100,
    PKT_SESSION_LIST = 101,

    PKT_SESSION_JOIN = 10,
    PKT_SESSION_LEAVE = 11,
    PKT_SESSION_CREATE = 12,
    PKT_SESSION_CLOSE = 13,
    PKT_SESSION_STATE = 14,        // <-

    PKT_READY = 15,             // ->
    PKT_START_SESSION = 16,        // ->
    PKT_START_SESSION_RETURN = 17, // <-
    PKT_SESSION_INFO = 18,
    PKT_INFO_RETURN = 19,
    PKT_REQUEST_EXTRA = 20,
    PKT_RESPONSE_EXTRA = 21,
    PKT_PHASE_RESULT = 22,
    PKT_SESSION_END = 23,
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


// PKT_SYNC_STATE
#pragma pack(push, 1)
typedef struct {
    uint32_t user_id;
    uint16_t token_length;
} pkt_sync_state_payload_t;
#pragma pack(pop)

// PKT_RECONNECT
#pragma pack(push, 1)
typedef struct {
    uint32_t user_id;
    uint16_t token_length;
} pkt_reconnect_payload_t;
#pragma pack(pop)

// PKT_SESSION_LIST
#pragma pack(push, 1)
typedef struct {
    uint16_t count;
} pkt_session_list_payload_t;
#pragma pack(pop)

// PKT_SESSION_JOIN
#pragma pack(push, 1)
typedef struct {
    uint32_t session_id;
} pkt_session_join_payload_t;
#pragma pack(pop)

// PKT_SESSION_STATE
#pragma pack(push, 1)
typedef struct {
    uint8_t nbrLign;
    uint8_t nbrCardsLign;
    uint8_t nbrCardsPlayer;
} pkt_session_state_payload_t;
#pragma pack(pop)


// PKT_SESSION_INFO
#pragma pack(push, 1)
typedef struct {
    int32_t  num;
    int32_t  numberHead;
    uint32_t client_id;
} pkt_card_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t hand_count;
} pkt_session_info_payload_t;
#pragma pack(pop)

#endif //PROTOCOL_H
