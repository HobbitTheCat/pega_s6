#include <stdint.h>
#include <stdio.h>

#include "Protocol/protocol.h"
#include "User/bot.h"

int bot_handle_session_info(bot_t* bot, const uint8_t* payload, const uint16_t payload_length) {
    if (!bot->user.game_initialized) return -1;

    if (payload_length < sizeof(pkt_session_info_payload_t)) return -1;

    const pkt_session_info_payload_t* hdr = (pkt_session_info_payload_t*)payload;
    const uint16_t hand_count = hdr->hand_count;
    const uint16_t player_count = hdr->player_count;
    const uint16_t board_length = bot->user.nb_line * bot->user.nb_card_line;

    const size_t expected =
        sizeof(pkt_session_info_payload_t) +
        (size_t)player_count * sizeof(pkt_player_score_t) +
        (size_t)board_length * sizeof(pkt_card_t) +
        (size_t)hand_count * sizeof(pkt_card_t);

    if (payload_length < expected) return -1;

    const pkt_player_score_t* scores = (const pkt_player_score_t*)(hdr + 1);
    const pkt_card_t* board_cards = (const pkt_card_t*)(scores + player_count);
    const pkt_card_t* hand_cards  = board_cards + board_length;

    row_t row[bot->user.nb_line];

    for (uint16_t r = 0; r < bot->user.nb_line; ++r) {
        int card_in_line = 0;
        int bulls_sum = 0;
        for (uint16_t c = 0; c < bot->user.nb_card_line; ++c) {
            const pkt_card_t* card = &board_cards[r * bot->user.nb_card_line + c];
            if (card->num != 0) {
                bot->placed_cards[card->num - 1] = 1;
                card_in_line++;
                bulls_sum += card->numberHead;
            } else {
                row[r].last_card = board_cards[r*bot->user.nb_card_line + c-1].num;
                row[r].bulls_sum = bulls_sum;
                row[r].count = card_in_line;
                break;
            }
        }
    }

    bot->index_row_to_take = -1;
    int best_head = INT16_MAX;
    for (int i = 0; i < bot->user.nb_line; i++) {
        if (row[i].bulls_sum < best_head) {best_head = row[i].bulls_sum;  bot->index_row_to_take = i;}
    }

    // const int
    int best_card_index = 0;
    if (bot->level == 0) best_card_index = bot_choose_card(hand_cards, hand_count, row, player_count);
    if (bot->level == 1) best_card_index = best_card_choice(bot->placed_cards, hand_cards, hand_count, player_count, board_cards, bot->user.nb_line, bot->user.nb_card_line, bot->user.max_card_value);
    else best_card_index = best_card_choice_deep(bot->placed_cards, hand_cards, hand_count, player_count, board_cards, bot->user.nb_line, bot->user.nb_card_line, bot->user.max_card_value, 3);
    printf("will play: %d\n", best_card_index);
    return bot_send_info_return(bot, best_card_index);
}

int calculate_danger(const int my_card, row_t* rows, const int player_count) {
    int best_row = -1;
    int min_diff = INT16_MAX;

    for (int i = 0; i < 4; i++) {
        if (my_card > rows[i].last_card) {
            const int diff = my_card - rows[i].last_card;
            if (diff < min_diff) {
                min_diff = diff;
                best_row = i;
            }
        }
    }

    if (best_row == -1) return 500;

    const row_t targe_row = rows[best_row];
    if (targe_row.count == 5) return 1000 + (targe_row.bulls_sum * 10);
    int danger = 0;
    if (targe_row.count == 4) if (min_diff > 1) danger += 50 * player_count;
    if (min_diff > player_count) danger += (min_diff * 2);
    return danger;
}

int bot_choose_card(const pkt_card_t* cards, const int hand_count, row_t* rows, const int player_count) {
    int best_card_index = -1;
    int min_danger = INT16_MAX;
    for (int i = 0; i < hand_count; ++i) {
        const int danger = calculate_danger(cards[i].num, rows, player_count);
        if (danger < min_danger) {
            min_danger = danger;
            best_card_index = i;
        }
    }
    return best_card_index;
}