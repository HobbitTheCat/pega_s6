#ifndef PLAYER_H
#define PLAYER_H
#include <stdint.h>

typedef struct {
    uint32_t player_id;

    uint64_t last_active_time;
} player_t;

int create_player(player_t* player, uint32_t client_id);

#endif //PLAYER_H
