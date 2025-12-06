#include <stdlib.h>

#include "Session/Game/game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/random.h>

int init_game(game_t* game, const uint8_t nbrLign, const uint8_t nbrCardsLign, const uint8_t nbrCardsPlayer, const uint8_t nbrCards, const uint8_t nbrHead,const uint8_t nb_player) {
    if (nbrCards <= (uint8_t)(nbrCardsPlayer * nb_player)) return -1;

    game->nbrLign = nbrLign;
    game->nbrCardsLign = nbrCardsLign;
    game->nbrCardsPlayer = nbrCardsPlayer;
    game->nbrCards = nbrCards;
    game->nbHead = nbrHead;
    game->nextCards = 0;
    game->board = malloc((size_t)game->nbrCardsLign * game->nbrLign * sizeof(card_t));
    if (!game->board) return -1;

    memset(game->board, 0, (size_t)game->nbrCardsLign * game->nbrLign * sizeof(card_t));
    return 0;
}

void cleanup_game(game_t* game) {
    free(game->board);
}

int distrib_cards(game_t* game, player_t* player, const int nb_player) {
    card_t* deck = malloc(game->nbrCards * sizeof(card_t));
    if (!deck) return -1;

    for (int i = 0; i < game->nbrCards; i++) {
        card_t card = {i+1, 0, 0};
        deck[i] = card;
    }
    melange_list(deck, game->nbrCards);
    melange_head(deck, game);
    melange_list(deck, game->nbrCards);

    for (int i = 0; i < game->nbrCardsLign * game->nbrLign; ++i) {
        if (i % game->nbrCardsLign == 0) {
            if (game->nextCards < game->nbrCards) {
                game->board[i] = deck[game->nextCards++];
            } else {
                game->board[i] = (card_t){0,0,0};
            }
        } else {
            game->board[i] = (card_t){0,0,0};
        }
    }

    for (int i=0; i<nb_player;i++) {
        if (player[i].player_id == 0) continue;
        if (game->nextCards + game->nbrCardsPlayer <= game->nbrCards) {
            memcpy(player[i].player_cards, deck + game->nextCards, (size_t)game->nbrCardsPlayer*sizeof(card_t));
            game->nextCards += game->nbrCardsPlayer;
        } else {
            printf("Player %d has not cards.\n", player[i].player_id);
        }
    }

    free(deck);
    return 0;
}

int melange_list(card_t* cards, const int length) {
    for (int i = length - 1; i > 0; i--) {
        unsigned int buffer;
        if (getrandom(&buffer, sizeof(buffer), 0) != sizeof(buffer)) return -1;
        const int nbrRandom = buffer % (i + 1);

        const card_t temp = cards[i];
        cards[i] = cards[nbrRandom];
        cards[nbrRandom] = temp;
    }
    return 0;
}

int melange_head(card_t* cards, const game_t* game) {
    if (game->nbHead == 0 || game->nbrCards == 0) {
        return -1;
    }
    int total_poids = game->nbHead * (game->nbHead + 1) / 2;
    int current_card_index = 0;
    int cartes_restantes = game->nbrCards;

    if (total_poids == 0) return -1;

    for (int h = 1; h <= game->nbHead; h++) {
        const int poids_actuel = game->nbHead - h + 1;
        int nbr_cartes_attribuer;

        if (h == game->nbHead) {
            nbr_cartes_attribuer = cartes_restantes;
        } else {
            nbr_cartes_attribuer = (game->nbrCards * poids_actuel) / total_poids;
            if (nbr_cartes_attribuer > cartes_restantes) {
                nbr_cartes_attribuer = cartes_restantes;
            }
        }

        for (int k = 0; k < nbr_cartes_attribuer; k++) {
            if (current_card_index < game->nbrCards) {
                cards[current_card_index].numberHead = h;
                current_card_index++;
            }
        }

        cartes_restantes -= nbr_cartes_attribuer;

        if (cartes_restantes <= 0) break;
    }

    return 0;
}