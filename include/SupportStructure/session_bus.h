#ifndef SESSION_BUS_H
#define SESSION_BUS_H

#include "spsc_buffer.h"
#include <stdint.h>

#define SP_MAX_FRAME 4096

typedef enum {SESSION_UNREGISTER, SESSION_UNREGISTERED, USER_CONNECTED, USER_DISCONNECTED, USER_UNREGISTER, USER_LEFT} system_message_type_t;
typedef enum {SYSTEM_MESSAGE, USER_MESSAGE} session_message_type_t;

typedef struct {
    system_message_type_t type;
    uint32_t session_id;
    uint32_t user_id;
} system_message_t;

typedef struct {
    uint32_t client_id;
    uint8_t packet_type;
    uint16_t payload_length;

    uint8_t buf[SP_MAX_FRAME];
} user_message_t;

typedef struct {
    session_message_type_t type;
    union {
        system_message_t system;
        user_message_t user;
    } data;
} session_message_t;

typedef struct {
    ring_buffer_t queue;
    int event_fd;
} bus_base_t;

typedef bus_base_t bus_read_t;
typedef bus_base_t bus_write_t;

typedef struct {
    uint32_t session_id;
    bus_read_t read;
    bus_write_t write;

    size_t read_capacity;
    size_t write_capacity;
} session_bus_t;

session_bus_t* create_session_bus(uint32_t capacity, uint32_t session_id);
void destroy_session_bus(session_bus_t* session_bus);

int session_bus_push(bus_base_t* bus, session_message_t* message);
void print_session_message(const session_message_t* msg);

#endif //SESSION_BUS_H
