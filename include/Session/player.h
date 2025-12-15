#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

typedef enum {
    ROLE_NONE = 0,
    ROLE_PLAYER = 1,
    ROLE_OBSERVER = 2,
} player_role_t;

typedef struct {
    uint32_t player_id;
    int* player_cards_id;

    uint8_t nb_head;
    player_role_t role;
    uint8_t is_connected;
} player_t;

int create_player(player_t* player, uint32_t client_id, uint8_t nb_cards);
int cleanup_player(player_t* player);

uint16_t calc_hand_count(const player_t* player, uint8_t max_card);

#endif //PLAYER_H
