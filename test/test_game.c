#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "../include/Session/Game/game.h"
#include "../include/Session/Game/card.h"
#include "../include/Session/player.h"

// void print_cards(const card_t* card) {
//     printf("[%d , %d], ", card->num, card->numberHead);
// }
//
// void print_player(const player_t* player, const int nb_card) {
//     printf("Player: id%d ", player->player_id);
//     for (int i =0; i < nb_card; i++) {
//         print_cards(&player->player_cards[i]);
//     }
//     printf("\n");
// }
//
// void print_tableau(game_t* game) {
//     printf("\n===== BOARD =====\n");
//
//     for (uint8_t r = 0; r < game->nbrLign; r++) {
//         printf("Row %d: ", r + 1);
//
//         for (uint8_t c = 0; c < game->nbrCardsLign; c++) {
//             const int index = r * game->nbrCardsLign + c;
//             const card_t* card = &game->board[index];
//
//             if (card->num == 0) {
//                 printf("[        ] ");
//             } else {
//                 printf("[%3d (%2d)] ", card->num, card->numberHead);
//             }
//         }
//         printf("\n");
//     }
//     printf("=================\n");
// }

int main() {
    // const int player_nb = 4;
    // const int card_number = 4;
    // player_t player[player_nb];
    //
    // for (int i = 0; i < player_nb; i++) {
    //     create_player(&player[i], i+1, card_number);
    // }
    //
    // game_t* game = (game_t*)malloc(sizeof(game_t));
    //
    // init_game(game, 5, 5, card_number, 104, 6, player_nb);
    // distrib_cards(game, player, player_nb);
    // for (int i =0; i < player_nb; i++) {
    //     print_player(&player[i], card_number);
    // }
    //
    // print_tableau(game);

    return 0;
}