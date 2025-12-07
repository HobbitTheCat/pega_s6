#include "Session/player.h"

#include <stdlib.h>
#include <string.h>

int create_player(player_t* player, const uint32_t client_id, const uint8_t nb_cards) {
    player->player_id = client_id;
    player->player_cards = malloc(nb_cards * sizeof(card_t));
    if (!player->player_cards) return -1;
    player->last_active_time = 0;
    memset(player->player_cards, 0, (size_t)nb_cards * sizeof(card_t));
    return 0;
}

int cleanup_player(player_t* player) {
    player->player_id = 0;
    free(player->player_cards);
    return 0;
}

uint16_t calc_hand_count(const player_t* player, const uint8_t max_card) {
    uint16_t count = 0;
    for (uint8_t i = 0; i < max_card; i++) {
        if (player->player_cards[i].num != 0) {
            count++;
        }
    }
    return count;
}