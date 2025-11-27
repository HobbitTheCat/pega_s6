#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>

typedef enum {
    PLAYER_STATE_CONNECT,
    PLAYER_STATE_IN_SESSION,
    CLIENT_STATE_DISCONNECT,
} player_state;

typedef struct {
    int fd; // нужно иметь возможность его менять при переподключении
    uint32_t client_id;
    uint32_t session_id;
    player_state state;

    uint64_t last_active_time;
} player_t;

#endif //PLAYER_H
