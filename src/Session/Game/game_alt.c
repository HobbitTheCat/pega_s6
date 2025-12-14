#include <stdio.h>

#include "Session/Game/game.h"

int take_line(game_t* game, player_t* player, const uint8_t numeroLigne, const uint8_t indexPlayer){
    if (game->card_ready_to_place[indexPlayer] == -1) {printf("Interesting\n");}
    for (int i = 0; i < game->nbrCardsLign; i++) {
        if (game->board[numeroLigne*game->nbrCardsLign + i] == -1) break;
        player[indexPlayer].nb_head += game->deck[game->board[numeroLigne*game->nbrCardsLign + i]].numberHead;
        game->board[numeroLigne*game->nbrCardsLign + i] = -1;
    }
    game->board[numeroLigne*game->nbrCardsLign] = game->card_ready_to_place[indexPlayer];

    game->card_ready_to_place[indexPlayer] = -1;
    return 0;
}

int place_card(game_t* game, player_t* player, const uint8_t capacity) {
    for (int step = 0; step < capacity; step++) {
        int min_card_index = -1;

        for (int i = 0; i < capacity; i++) {
            if (game->card_ready_to_place[i] == -1) continue;
            if (min_card_index == -1 || game->card_ready_to_place[i] < game->card_ready_to_place[min_card_index]) {
                min_card_index = i;
            }
        }

        if (min_card_index == -1) break;
        const int current_card_value = game->card_ready_to_place[min_card_index];
        int best_row = -1;
        int min_diff = game->nbrCards + 100;
        int targe_coll_index = -1;
        for (int i = 0; i < game->nbrLign; i++) {
            int last_card_in_row = -1;
            int last_col = -1;
            for (int j = 0; j < game->nbrCardsLign; j++) {
                const int value = game->board[i*game->nbrCardsLign + j];
                if (value == -1) break;
                last_card_in_row = value;
                last_col = j;
            }
            if (last_card_in_row == -1) continue;
            if (current_card_value > last_card_in_row) {
                const int diff = current_card_value - last_card_in_row;
                if (diff < min_diff) {
                    min_diff = diff;
                    best_row = i;
                    targe_coll_index = last_col + 1;
                }
            }
        }

        if (best_row == -1) {
            game->game_state = WAITING_EXTRA;
            const uint32_t player_id = player[min_card_index].player_id;
            return (int)player_id;
        }
        if (targe_coll_index >= game->nbrCardsLign) {
            take_line(game, player, best_row, min_card_index);
        } else {
            game->board[best_row*game->nbrCardsLign + targe_coll_index] = current_card_value;
        }
        game->card_ready_to_place[min_card_index] = -1;
    }
    game->card_ready_to_place_count = 0;
    for (int i = 0; i < capacity; i++) {
        if (player[i].player_id != 0) {
            if (checking_cards(game, &player[i])) {
                game->game_state = INACTIVE;
                return -3;
            }
        }
    }
    game->game_state = WAITING;
    return -2;
}

