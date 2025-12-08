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
    if (nbrCards <= (uint8_t)(nbrCardsPlayer * nb_player)) return -1;

    game->nbrLign = nbrLign;
    game->nbrCardsLign = nbrCardsLign;
    game->nbrCardsPlayer = nbrCardsPlayer;
    game->nbrCards = nbrCards;
    game->nbHead = nbrHead;
    game->game_started = 0;
    game->card_ready_to_place_count = 0;

    game->nextCards = 0; // TODO rewrite

    game->deck = malloc(game->nbrCards * sizeof(card_t));
    if (!game->deck) return -1;

    game->board = malloc((size_t)game->nbrCardsLign * game->nbrLign * sizeof(int));
    if (!game->board) {free(game->deck); return -1;}
    memset(game->board, -1, (size_t)game->nbrCardsLign * game->nbrLign * sizeof(int));

    game->card_ready_to_place = malloc(nb_player*sizeof(int));
    if (!game->card_ready_to_place) {free(game->deck); free(game->board); return -1;}
    memset(game->card_ready_to_place, -1, nb_player*sizeof(int));
    return 0;

}

void cleanup_game(game_t* game) {
    free(game->board);
    free(game->deck);
    free(game->card_ready_to_place);
}

int distrib_cards(game_t* game, player_t* player, const int nb_player) {
    game->game_started = 1;

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
            printf("Player %d has not cards.\n", player[i].player_id);
        }
    }

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

int melange_head(int* ids, const game_t* game) {
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
                game->deck[ids[current_card_index]].numberHead = h;
                current_card_index++;
            }
        }

        cartes_restantes -= nbr_cartes_attribuer;

        if (cartes_restantes <= 0) break;
    }

    return 0;
}

void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void bubbleSort(int* arr, int n) {
    int i, j;
    int swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = 0;
        for (j = 0; j < n - i - 1; j++) {

            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }
        if (swapped == 0)
            break;
    }
}

int placement_card(game_t* game, player_t* player, const uint8_t capacity){

    int card_index = 0;
    int best_diff = game->nbrCards+1;
    int bestrow = 0;
    bubbleSort(game->card_ready_to_place,capacity);

    int i = 0;
    for (int k=0;k<capacity;k++){
        if (game->card_ready_to_place[k] != -1) {
            for (i=0;i<game->nbrLign;i++) {
                for (int j=0;j<game->nbrCardsLign;j++){
                    const int val = (i*game->nbrCardsLign)+j;
                    int is_end_of_line = (j == game->nbrCardsLign - 1);
                    if (game->board[val] > game->card_ready_to_place[k] || (!is_end_of_line && game->board[val+1] != -1)) continue;

                    if (game->card_ready_to_place[k] - game->board[val] < best_diff && game->board[val] != -1 ) {
                        best_diff = game->card_ready_to_place[k] - game->board[val];
                        card_index = val;
                        bestrow = i;
                    }
                }
            }
            if (best_diff != game->nbrCards+1) {
                if (game->board[card_index +1] == -1 && game->board[card_index +1] % game->nbrCardsLign != 0) {
                    game->board[card_index +1] = game->card_ready_to_place[k];
                }
                else {

                    takeLigne(game,player,bestrow,k,capacity);
                }
            }
            else {
                const uint32_t player_id = game->deck[game->card_ready_to_place[k]].client_id;
                int index = 0;
                for (int j = 0; j < capacity; j++) {if (player_id == player[j].player_id) break;index++;}
                return (int)player_id;
            }
        }
        best_diff =  game->nbrCards+1;
        card_index = 0;
        bestrow = 0;
    }
    game->card_ready_to_place_count = 0;
    for (int i = 0; i<capacity;i++) {
        if (player[i].player_id != 0) {
            if (checking_cards(game,&player[i])) {
                return -3;
            }
        }
    }
    return -2;
}


int takeLigne(game_t* game, player_t* player, const uint8_t numeroLigne, const uint8_t indexPlayer, const uint8_t capacity){
    const uint32_t player_id = game->deck[game->card_ready_to_place[indexPlayer]].client_id;
    int index = 0;
    for (int i =0; i < capacity; i++) {if (player_id == player[i].player_id) break;index++;}
    printf("player_id %d \n", index);
    for (int i=0;i<game->nbrCardsLign;i++) {
        if (game->board[numeroLigne*game->nbrCardsLign+i] == -1) continue;
        player[index].nb_head += game->deck[game->board[numeroLigne*game->nbrCardsLign+i]].numberHead;
        
        if (i==0)
            game->board[numeroLigne*game->nbrCardsLign+i] = game->card_ready_to_place[indexPlayer];
        else
            game->board[numeroLigne*game->nbrCardsLign+i] = -1;
    }
    game->card_ready_to_place[indexPlayer] = -1;
    return 0;
}
int checking_cards(game_t* game,player_t* player) {
     for (int i=0;i<game->nbrCardsPlayer;i++) {
         if (player->player_cards_id[i] != -1) return 0;
     }
      return 1;
}

