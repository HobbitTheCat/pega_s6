#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>

#include "SupportStructure/session_bus.h"
#include "Session/player.h"

typedef struct {
    uint32_t id;

    player_t* players;
    uint32_t id_creator;
    size_t capacity;
    size_t num_players;

    session_bus_t* bus;
    int epoll_fd;
    volatile int running;

    void* game_state;
    // TODO a completer
} session_t;

int init_session(session_t* session, uint32_t session_id, uint8_t capacity);
void cleanup_session_fist_step(const session_t* session);
void cleanup_session(session_t* session);

void* session_main(void* arg);
int handle_system_message(session_t* session, session_message_t* msg);
int send_user_response(const session_t* session, const session_message_t* msg, uint16_t packet_size, const uint8_t* buffer);

int get_index_by_id(const session_t* session, uint32_t user_id);
int get_first_free_player_place(const session_t* session);

int add_player(session_t* session, int fd, uint32_t client_id);
int remove_player(session_t* session, uint32_t player_id);
#endif //SESSION_H
