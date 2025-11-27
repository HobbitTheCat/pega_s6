#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <stddef.h>

#include "SupportStructure/session_bus.h"
#include "Session/player.h"

typedef struct {
    uint32_t id;
    player_t* players;
    size_t num_players;

    session_bus_t* bus;
    int epoll_fd;
    volatile int running;

    void* game_state;
    // TODO a completer
} session_t;

int init_session(session_t* session, uint32_t session_id, uint8_t number_of_players);
void cleanup_session(session_t* session);

void* session_main(void* arg);

#endif //SESSION_H
