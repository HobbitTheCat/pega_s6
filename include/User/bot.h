#ifndef BIT_H
#define BIT_H
#include "User/user.h"

typedef struct {
    int last_card;
    int count;
    int bulls_sum;
} row_t;

typedef struct {
    user_t user;

    uint32_t session_id;
    int index_row_to_take;
    int is_creator;
} bot_t;

int bot_init(bot_t* user);
void bot_cleanup(bot_t* bot);

int bot_connect(bot_t* bot, const char* host, uint16_t port);
int bot_start(bot_t* bot, const char* host, uint16_t port, uint32_t session_id);
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
int bot_handle_session_status(bot_t* bot, const uint8_t* payload, uint16_t payload_length);
int bot_handle_session_state(bot_t* bot, const uint8_t* payload, uint16_t payload_length);

// Game
int calculate_danger(int my_card, row_t* rows, int player_count);
int bot_choose_card(const pkt_card_t* cards, int hand_count, row_t* rows, int player_count);

#endif //BIT_H
