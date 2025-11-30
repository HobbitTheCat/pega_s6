#include "Session/session.h"

int create_player(player_t* player, const uint32_t fd,const uint32_t client_id) {
    player->fd = fd;
    player->client_id = client_id;
    return 0;
}

int add_player(session_t* session, const int fd, const uint32_t client_id) {
    player_t player;
    create_player(&player, fd, client_id);
    session->players[get_first_free_player_place(session)] = player;
    session->num_players++;
    return 0;
}

int remove_player(session_t* session, const uint32_t player_id) {
    const int player_index = get_index_by_id(session, player_id);
    if (player_index == -1) return -1;
    session->players[player_index].client_id = 0;
    session->num_players--;
    return 0;
}