#ifndef SESSION_H
#define SESSION_H

#include <stddef.h>
#include <stdint.h>
#include <sys/poll.h>
#include <stdatomic.h>

#include "log_bus.h"
#include "SupportStructure/session_bus.h"
#include "Session/Game/game.h"
#include "Session/player.h"

typedef enum { START, ACTION, EXTR, PHASE_RESULT, RESULT } event_type_s;

typedef struct {
    uint32_t id;
    uint8_t is_visible;

    uint32_t id_creator;

    player_t* players;
    size_t max_clients;     // Size of players
    size_t game_capacity;   // Max players by rules

    size_t number_clients;  // All clients (including observers)
    size_t active_players;  // ROLE_PLAYER

    session_bus_t* bus;
    log_bus_t* log_bus;
    int timer_fd;
    int unregister_send;
    _Atomic int running;

    game_t* game;
} session_t;

int init_session(session_t* session, log_bus_t* log_bus, uint32_t session_id, uint8_t capacity, uint8_t is_visible);
void send_unregister(session_t* session);
void cleanup_session(session_t* session);

void* session_main(void* arg);
int handle_system_message(session_t* session, session_message_t* msg);
int handle_result_of_play(session_t* session, int result);
int handle_game_message(session_t* session, const session_message_t* msg);

int session_send_to_player(const session_t* session, uint32_t user_id, uint8_t packet_type, const void* payload, uint16_t payload_length);
int send_system_message_to_server(const session_t* session, uint32_t user_id, system_message_type_t type);

int get_index_by_id(const session_t* session, uint32_t user_id);
int get_first_free_player_place(const session_t* session);

int add_player(session_t* session, uint32_t client_id, player_role_t role);
int change_player_role(session_t* session, uint32_t player_id, player_role_t new_role);
int remove_player(session_t* session, uint32_t client_id);

// Packets
int session_send_simple_packet(const session_t* session, uint32_t user_id, uint8_t packet_type);
int session_send_error_packet(const session_t* session, uint32_t user_id, uint8_t error_code, const char* error_message);
int session_send_state(const session_t* session, uint32_t user_id);
int session_send_info(const session_t* session, uint32_t user_id);
int session_send_phase_result(const session_t* session, uint32_t user_id);

int session_handle_set_rules(session_t* session, session_message_t* msg);
int session_handle_add_bot(const session_t* session, const session_message_t* msg);

int start_move(const session_t* session);
int game_handle_info_return(session_t* session, const session_message_t* msg);
int game_handle_response_extra(session_t* session, const session_message_t* msg);
int game_send_request_extra(session_t* session, uint32_t user_id);

// Log
int append(char* buf, int pos, const char* fmt, ...);
int push_modification_log(const session_t* session, event_type_s action);
int push_extra_log(const session_t* session, const int player_id, const int line);

#endif //SESSION_H
