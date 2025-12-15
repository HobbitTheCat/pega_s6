#include "Session/player.h"

#include <stdlib.h>
#include <string.h>

int create_player(player_t* player, const uint32_t client_id, const uint8_t nb_cards) {
    player->player_id = client_id;
    if (nb_cards > 0) {
        player->player_cards_id = malloc(nb_cards * sizeof(int));
        if (!player->player_cards_id) return -1;
    }
    else player->player_cards_id = NULL;

    return 0;
}

int cleanup_player(player_t* player) {
    player->player_id = 0;
    if (player->player_cards_id) free(player->player_cards_id);
    return 0;
}

uint16_t calc_hand_count(const player_t* player, const uint8_t max_card) {
    uint16_t count = 0;
    for (uint8_t i = 0; i < max_card; i++) {
        if (player->player_cards_id[i] != -1) {
            count++;
        }
    }
    return count;
}