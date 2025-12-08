#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

typedef struct {
    uint32_t player_id;
    int* player_cards_id;

    uint64_t last_active_time;
    uint8_t nb_head;
} player_t;

int create_player(player_t* player, uint32_t client_id, uint8_t nb_cards);
int cleanup_player(player_t* player);

uint16_t calc_hand_count(const player_t* player, uint8_t max_card);

#endif //PLAYER_H
