#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "game.h"
#include "User/bot.h"

int bot_handle_packet(bot_t* bot, const uint8_t type, const uint8_t* payload, const uint16_t payload_length) {
    sleep(1);
    switch (type) {
        case PKT_WELCOME: bot_send_simple(bot, PKT_REGISTER);return 0;
        case PKT_SYNC_STATE:
            printf("Got pkt\n");
            bot_handle_sync_state(bot, payload, payload_length);
            if (bot->session_id > 0) bot_send_join_session(bot);
            else {
                bot->is_creator = 1;
                printf("Creating session\n");
                user_send_create_session(&bot->user, 4, 0);
            }
            return 0;
        case PKT_SESSION_STATE:
            return bot_handle_session_state(bot, payload, payload_length);
        case PKT_SESSION_INFO:
            return bot_handle_session_info(bot, payload, payload_length);
        case PKT_REQUEST_EXTRA:
            return bot_send_resp_extra(bot);
        case PKT_ERROR:
            printf("Error\n");
        case PKT_PHASE_RESULT: //TODO delete phase result here
            printf("Got phase result\n");
        case PKT_SESSION_END:
            bot_send_simple(bot, PKT_SESSION_LEAVE);
            sleep(1);
            bot_send_simple(bot, PKT_UNREGISTER);
            bot_handle_session_disconnect(bot);
            bot->user.is_running = 0;
            user_close_connection(&bot->user);
            return 0;
        default:
            printf("Got packet %d\n", type); return 0;
    }
}

int bot_send_simple(bot_t* bot, const uint8_t type) {
    return user_send_simple(&bot->user, type);
}
int bot_send_join_session(bot_t* bot) {
    return user_send_join_session(&bot->user, bot->session_id);
}
int bot_send_info_return(bot_t* bot, const uint8_t card_choice) {
    return user_send_info_return(&bot->user, card_choice);
}
int bot_send_resp_extra(bot_t* bot) {
    return user_send_response_extra(&bot->user, (int16_t)bot->index_row_to_take);
}


int bot_handle_sync_state(bot_t* bot, const uint8_t* payload, const uint16_t payload_length) {
    client_handle_sync_state(&bot->user, payload, payload_length);
    printf("Bot id %d\n", bot->user.client_id);
    return 0;
}

int bot_handle_session_state(bot_t* bot, const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_session_state_payload_t)) return -1;

    const pkt_session_state_payload_t* pkt = (pkt_session_state_payload_t*) payload;
    user_join_session(&bot->user, pkt->max_card_value, pkt->nbrLign, pkt->nbrCardsLign, pkt->nbrCardsPlayer);
    bot->session_id = pkt->session_id;
    if (bot->placed_cards != NULL) free(bot->placed_cards);
    bot->placed_cards = calloc(pkt->max_card_value, sizeof(int));
    return 0;
}

int bot_handle_session_disconnect(bot_t* bot) {
    if (bot->session_id == 0) return 0;
    free(bot->placed_cards);
    bot->placed_cards = NULL;
    bot->session_id = 0;
    return 0;
}