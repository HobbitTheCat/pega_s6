#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#include "Session/player.h"
#include "Game/card.h"

typedef struct {
    uint8_t nbrLign;
    uint8_t nbrCardsLign;
    uint8_t nbrCardsPlayer;
    uint8_t nbrCards;
    uint8_t nbHead;
    uint8_t nextCards;

    int game_started;

    card_t* deck;
    int* board;
    int* card_ready_to_place;
    int card_ready_to_place_count;
} game_t;


int init_game(game_t* game, uint8_t nbrLign, uint8_t nbrCardsLign, uint8_t nbrCardsPlayer, uint8_t nbrCards, uint8_t nbrHead, uint8_t nb_player);
void cleanup_game(game_t* game);

int distrib_cards(game_t* game, player_t* player,int nb_player);
int melange_ids(int* ids, int length);
int melange_head(int* ids, const game_t* game);

int placement_card(game_t* game, player_t* player,uint8_t capacity);
void swap(int* a, int* b);
void bubbleSort(int* arr, int n);
int takeLigne(game_t* game, player_t* player, uint8_t numeroLigne,uint8_t indexPlayer,uint8_t capacity);
// int takeLigne(game_t* game, player_t* player, int row_idx, int card_val);

#endif //GAME_H
