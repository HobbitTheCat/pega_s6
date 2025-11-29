#include "Session/player.h"

int create_player(player_t* player, const uint32_t fd,const uint32_t client_id) {
    player->fd = fd;
    player->client_id = client_id;
    return 0;
}