#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#include "SupportStructure/log_bus.h"
#include "Session/player.h"
#include "Game/card.h"

typedef enum {INACTIVE = 1, WAITING = 2, WAITING_EXTRA = 3, PLAYING = 4} state_t;

typedef struct {
    uint8_t nbrLign;
    uint8_t nbrCardsLign;
    uint8_t nbrCardsPlayer;
    uint8_t nbrCards;
    uint8_t nbHead;
    uint8_t nextCards;

    uint32_t extra_wait_from;

    state_t game_state;

    card_t* deck;
    int* board;
    int* card_ready_to_place;
    int card_ready_to_place_count;

} game_t;


int init_game(game_t* game, uint8_t nbrLign, uint8_t nbrCardsLign, uint8_t nbrCardsPlayer, uint8_t nbrCards, uint8_t nbrHead, uint8_t nb_player, uint8_t game_capacity);
void cleanup_game(const game_t* game);

int distrib_cards(game_t* game, const player_t* player, int nb_player, log_bus_t* bus, int session_id);
int melange_ids(int* ids, int length);
int melange_head(const int* ids, const game_t* game);

// int placement_card(game_t* game, player_t* player,uint8_t capacity);
// void swap(int* a, int* b);
// void bubbleSort(int* arr, int n);
// int takeLigne(game_t* game, player_t* player, uint8_t numeroLigne,uint8_t indexPlayer,uint8_t capacity);
int checking_cards(game_t* game,player_t* player);

int take_line(game_t* game, player_t* player, uint8_t numeroLigne, uint8_t indexPlayer);
int place_card(game_t* game, player_t* player, uint8_t capacity);

#endif //GAME_H
