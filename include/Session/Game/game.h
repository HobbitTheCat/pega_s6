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
    card_t* board;
} game_t;


int init_game(game_t* game, uint8_t nbrLign, uint8_t nbrCardsLign, uint8_t nbrCardsPlayer, uint8_t nbrCards, uint8_t nbrHead, uint8_t nb_player);
int distrib_cards(game_t* game, player_t* player,int nb_player);
int melange_list(card_t* cards, int length);
int melange_head(card_t* cards, const game_t* game);
void cleanup_game(game_t* game);


#endif //GAME_H