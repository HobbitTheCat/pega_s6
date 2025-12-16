#ifndef BIT_H
#define BIT_H
#include "User/user.h"

typedef struct {
    int last_card;
    int count;
    int bulls_sum;
} row_t;

typedef enum {
    BOT_STATE_CONNECTING,
    BOT_STATE_IDLE,
    BOT_STATE_WAITING_TIMER,
    BOT_STATE_DISCONNECTED
} bot_state_machine_t;

typedef struct {
    uint8_t level;

    user_t user;
    uint32_t session_id;
    int index_row_to_take;
    int is_creator;

    bot_state_machine_t state;
    time_t next_action_time;
    int id;

    int* placed_cards;
} bot_t;

int bot_init(bot_t* user);
void bot_cleanup(bot_t* bot);

int bot_connect(bot_t* bot, const char* host, uint16_t port);
int bot_start(bot_t* bot, const char* host, uint16_t port, uint32_t session_id, uint8_t level);
int bot_run(bot_t* bot);

int bot_handle_read(bot_t* bot);
int bot_handle_write(bot_t* bot);

int bot_handle_packet(bot_t* bot, uint8_t type, const uint8_t* payload, uint16_t payload_length);

int bot_send_simple(bot_t* bot, uint8_t type);
int bot_send_join_session(bot_t* bot);
int bot_send_info_return(bot_t* bot, uint8_t card_choice);
int bot_handle_session_info(bot_t* bot, const uint8_t* payload, uint16_t payload_length);
int bot_send_resp_extra(bot_t* bot);

int bot_handle_sync_state(bot_t* bot, const uint8_t* payload, uint16_t payload_length);
int bot_handle_session_state(bot_t* bot, const uint8_t* payload, uint16_t payload_length);
int bot_handle_session_disconnect(bot_t* bot) ;

// Game
int calculate_danger(int my_card, row_t* rows, int player_count);
int bot_choose_card(const pkt_card_t* cards, int hand_count, row_t* rows, int player_count);

// Advanced
uint32_t fast_rand();
void shuffle(int* array, int len);
int simple_enemy_choice(const int* last_in_row, const int* cards_in_row, int row_number, int row_length, const int* hand_cards, int hand_length);


typedef struct {
    int last_in_row[4];
    int row_penalty[4];
    int cards_in_row[4];
    int row_number;
    int row_length;
    int num_players;
    int our_card_choice;
} GameState;

int best_card_choice_deep(const int* placed_cards, const pkt_card_t* my_hand, int number_of_cards_in_hand, int number_of_players,
    const pkt_card_t* board, int row_number, int row_length, int max_value, int depth);

int simulate_full_game(GameState* state, int** player_hands,int cards_remaining, int depth);

int simulation_step(int* last_in_row, int* number_of_cards_in_row, int* row_penalty, int row_number, int row_length, int* played_cards, int number_of_players) ;
int best_card_choice(const int* placed_cards, const pkt_card_t* my_hand, int number_of_cards_in_hand, int number_of_players, const pkt_card_t* board, int row_number, int row_length, int max_value);
#endif //BIT_H
