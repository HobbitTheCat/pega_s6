#include <stdlib.h>

#include "Session/Game/game.h"
#include "Session/session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/random.h>

// TODO add function to check that game is valid

int init_game(game_t* game, const uint8_t nbrLign, const uint8_t nbrCardsLign, const uint8_t nbrCardsPlayer, const uint8_t nbrCards, const uint8_t nbrHead,const uint8_t nb_player) {
    if (nbrCards <= (uint8_t)(nbrCardsPlayer * nb_player)) { return -1;
    }

    game->nbrLign = nbrLign;
    game->nbrCardsLign = nbrCardsLign;
    game->nbrCardsPlayer = nbrCardsPlayer;
    game->nbrCards = nbrCards;
    game->nbHead = nbrHead;
    game->game_state = INACTIVE;
    game->card_ready_to_place_count = 0;

    game->nextCards = 0; // TODO rewrite

    game->deck = malloc(game->nbrCards * sizeof(card_t));

    if (!game->deck) { return -1;}

    game->board = malloc((size_t)game->nbrCardsLign * game->nbrLign * sizeof(int));
    if (!game->board) {; free(game->deck); return -1;}
    memset(game->board, -1, (size_t)game->nbrCardsLign * game->nbrLign * sizeof(int));

    game->card_ready_to_place = malloc(nb_player*sizeof(int));
    if (!game->card_ready_to_place) { free(game->deck); free(game->board); return -1;}
    memset(game->card_ready_to_place, -1, nb_player*sizeof(int));
    return 0;

}

void cleanup_game(const game_t* game) {
    free(game->board);
    free(game->deck);
    free(game->card_ready_to_place);
}

int distrib_cards(game_t* game, const player_t* player, const int nb_player, log_bus_t* bus, const int session_id) {
    for (int i = 0; i < game->nbrCards; i++) {
        card_t card = {i+1, 0, 0};
        game->deck[i] = card;
    }

    int ids[game->nbrCards];
    for (int i = 0; i < game->nbrCards; i++) ids[i] = i;
    melange_ids(ids, game->nbrCards);
    melange_head(ids, game);
    melange_ids(ids, game->nbrCards);

    for (int i = 0; i < game->nbrCardsLign * game->nbrLign; ++i) {
        if (i % game->nbrCardsLign == 0) {
            if (game->nextCards < game->nbrCards) {
                game->board[i] = ids[game->nextCards++];
            } else {
                game->board[i] = -1;
            }
        } else {
            game->board[i] = -1;
        }
    }

    for (int i=0; i<nb_player;i++) {
        if (player[i].player_id == 0) continue;
        if (game->nextCards + game->nbrCardsPlayer <= game->nbrCards) {
            memcpy(player[i].player_cards_id, ids + game->nextCards, (size_t)game->nbrCardsPlayer*sizeof(int));
            game->nextCards += game->nbrCardsPlayer;
        } else {
            log_bus_push_param(bus, session_id,LOG_WARN,"Player %d has no cards.", player[i].player_id);
        }
    }
    game->game_state = WAITING;
    return 0;
}

int melange_ids(int* ids, const int length) {
    for (int i = length - 1; i > 0; i--) {
        unsigned int buffer;
        if (getrandom(&buffer, sizeof(buffer), 0) != sizeof(buffer)) return -1;
        const int nbrRandom = (int)(buffer % (unsigned)(i + 1));

        const int temp = ids[i];
        ids[i] = ids[nbrRandom];
        ids[nbrRandom] = temp;
    }
    return 0;
}

int melange_head(const int* ids, const game_t* game) {
    if (game->nbHead == 0 || game->nbrCards == 0) {
        return -1;
    }
    const int total_poids = game->nbHead * (game->nbHead + 1) / 2;
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
                game->deck[ids[current_card_index]].numberHead = h;
                current_card_index++;
            }
        }

        cartes_restantes -= nbr_cartes_attribuer;

        if (cartes_restantes <= 0) break;
    }

    return 0;
}


int checking_cards(game_t* game,player_t* player) {
     for (int i=0;i<game->nbrCardsPlayer;i++) {
         if (player->player_cards_id[i] != -1) return 0;
     }
      return 1;
}

