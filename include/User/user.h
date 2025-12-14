#ifndef USER_H
#define USER_H

#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdatomic.h>

#include "Protocol/proto_io.h"

#define RECONNECT_TOKEN_MAX 256

typedef struct {
    uint8_t bot;
    uint32_t client_id;
    char reconnection_token[RECONNECT_TOKEN_MAX];

    int sockfd;
    struct sockaddr_in server_addr;

    struct pollfd fds[2];
    nfds_t nfds;

    rx_state_t rx;
    tx_state_t tx;

    int want_pollout;
    _Atomic int is_running;

    // Session
    uint16_t max_card_value;
    uint8_t nb_line;
    uint8_t nb_card_line;
    uint8_t nb_card_player;
    int game_initialized;

    uint8_t debug_mode;
} user_t;

int user_init(user_t* user);
void user_cleanup(user_t* user);

int user_connect(user_t* user, const char* host, uint16_t port);
void user_close_connection(user_t* user);

// void user_main_loop(user_t* user);
void user_run(user_t* user, const char* host, uint16_t port);
int user_loop_once(user_t* user);

int user_handle_read(user_t* user);
void user_execute_command(user_t* user, const char* cmd);
void user_handle_stdin(user_t* user);
int user_handle_write(user_t* user);

int user_enqueue_message(user_t* user, const uint8_t* buffer, uint32_t total_length);
int user_send_packet(user_t* user, uint8_t type, const void* payload, uint16_t payload_length);

int user_handle_packet(user_t* user, uint8_t type, const uint8_t* payload, uint16_t payload_length);

// Session
int user_join_session(user_t* user, uint16_t max_card_value, uint8_t nb_line, uint8_t nb_card_line, uint8_t nb_card_player) ;
int user_quit_session(user_t* user);

// Packets
int user_send_simple(user_t* user, uint8_t type);
int user_send_reconnect(user_t* user);
int user_send_create_session(user_t* user, uint8_t number_players, uint8_t is_visible);
int user_send_join_session(user_t* user, uint32_t session_id);
int user_send_add_bot(user_t* user, const uint8_t number_of_bot_to_add, uint8_t bot_difficulty);
int user_send_info_return(user_t* user, uint8_t card_choice);
int user_send_response_extra(user_t* user, int16_t row_index);
int user_send_set_session_rules(user_t* user, uint8_t is_visible, uint8_t nb_lines,
        uint8_t nb_cards_line, uint8_t nb_cards, uint8_t nb_cards_player, uint8_t max_heads);

int client_handle_request_extra(user_t* user, const uint8_t* payload, uint16_t payload_length);
int client_handle_sync_state(user_t* user, const uint8_t* payload, uint16_t payload_length);
int client_handle_session_list(const uint8_t* payload, uint16_t payload_length);
int client_handle_session_state(user_t* user, const uint8_t* payload, uint16_t payload_length);
int client_handle_session_info(user_t* user, const uint8_t* payload, uint16_t payload_length);
int client_handle_phase_result(const user_t* user, const uint8_t* payload, uint16_t payload_length);
#endif //USER_H
