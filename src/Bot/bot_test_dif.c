#include "User/bot.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int simulate_full_game(GameState* state, int** player_hands,
                       int cards_remaining, int depth) {
    if (cards_remaining == 0 || depth < 0) {
        return 0;
    }

    int played_cards[state->num_players];

    // Выбор карт всеми игроками
    for (int p = 0; p < state->num_players; p++) {
        if (p == 0 && depth == 0) {
            // Наш выбор на первом уровне задан извне
            played_cards[0] = state->our_card_choice;
        } else {
            played_cards[p] = simple_enemy_choice(
                state->last_in_row,
                state->cards_in_row,
                state->row_number,
                state->row_length,
                player_hands[p],
                cards_remaining
            );
        }
    }

    // Копируем состояние для симуляции
    GameState next_state = *state;
    int played_cards_copy[state->num_players];
    memcpy(played_cards_copy, played_cards, state->num_players * sizeof(int));

    int penalty = simulation_step(
        next_state.last_in_row,
        next_state.cards_in_row,
        next_state.row_penalty,
        next_state.row_number,
        next_state.row_length,
        played_cards_copy,
        next_state.num_players
    );

    // Рекурсивно симулируем следующий раунд
    if (depth > 0) {
        penalty += simulate_full_game(&next_state, player_hands,
                                      cards_remaining - 1, depth - 1);
    }

    return penalty;
}

int best_card_choice_deep(const int* placed_cards, const pkt_card_t* my_hand,
                          int number_of_cards_in_hand,
                          int number_of_players,
                          const pkt_card_t* board,
                          int row_number,
                          int row_length,
                          int max_value,
                          int depth) {  // Новый параметр!

    int total_penalty[number_of_cards_in_hand];
    memset(total_penalty, 0, number_of_cards_in_hand * sizeof(int));
    const int simulations = 1000;

    int all_cards[max_value];
    memset(all_cards, 0, max_value * sizeof(int));
    memcpy(all_cards, placed_cards, max_value*sizeof(int));

    int last_in_row[row_number];
    int row_penalty[row_number];
    memset(row_penalty, 0, row_number*sizeof(int));
    int number_of_cards_in_row[row_number];
    memset(number_of_cards_in_row, 0, row_number*sizeof(int));
    for (int row = 0; row < row_number; row++) {
        for (int c = 0; c < row_length; c++) {
            const int number_on_card = board[row*row_length + c].num;
            if (number_on_card == 0) {
                last_in_row[row] = board[row * row_length + c - 1].num;
                break;
            }
            number_of_cards_in_row[row] += 1;
            all_cards[number_on_card - 1] = 1;
            row_penalty[row] += board[row*row_length + c].numberHead;
        }
    }

    int hand[number_of_cards_in_hand];
    for (int c = 0; c < number_of_cards_in_hand; c++) {
        if (my_hand[c].num == 0) continue;
        hand[c] = my_hand[c].num;
        all_cards[hand[c] - 1] = 1;
    }


    int unknow_card_count = 0;
    for (int c = 0; c < max_value; c++)
        if (all_cards[c] == 0) unknow_card_count++;
    printf("Unknown card count %d\n", unknow_card_count);

    int unknown_cards[unknow_card_count];
    int index = 0;
    for (int c = 0; c < max_value; c++) {
        if (all_cards[c] == 1) continue;
        unknown_cards[index] = c+1;
        index++;
    }

    // Для каждой симуляции
    for (int sim = 0; sim < simulations; sim++) {
        shuffle(unknown_cards, unknow_card_count);

        // Раздаём карты противникам
        int* player_hands[number_of_players];
        player_hands[0] = hand;  // Наша рука известна

        for (int p = 1; p < number_of_players; p++) {
            player_hands[p] = unknown_cards + (p - 1) * number_of_cards_in_hand;
        }

        // Тестируем каждую нашу карту
        for (int i = 0; i < number_of_cards_in_hand; i++) {
            if (hand[i] == 0) continue;

            // Копируем руки игроков
            int** hands_copy = malloc(number_of_players * sizeof(int*));
            for (int p = 0; p < number_of_players; p++) {
                hands_copy[p] = malloc(number_of_cards_in_hand * sizeof(int));
                memcpy(hands_copy[p], player_hands[p],
                       number_of_cards_in_hand * sizeof(int));
            }

            // Настраиваем начальное состояние
            GameState state = {
                .row_number = row_number,
                .row_length = row_length,
                .num_players = number_of_players,
                .our_card_choice = hand[i]
            };
            memcpy(state.last_in_row, last_in_row, row_number * sizeof(int));
            memcpy(state.row_penalty, row_penalty, row_number * sizeof(int));
            memcpy(state.cards_in_row, number_of_cards_in_row, row_number * sizeof(int));

            // Симулируем игру на заданную глубину
            total_penalty[i] += simulate_full_game(&state, hands_copy,
                                                   number_of_cards_in_hand, depth);

            // Освобождаем память
            for (int p = 0; p < number_of_players; p++) {
                free(hands_copy[p]);
            }
            free(hands_copy);
        }
    }

    // Находим карту с минимальным штрафом
    int min_penalty = total_penalty[0];
    int best_card_index = 0;
    printf("Penalty: %d", min_penalty);
    for (int i = 1; i < number_of_cards_in_hand; i++) {
        printf(", %d", total_penalty[i]);
        if (hand[i] != 0 && total_penalty[i] < min_penalty) {
            min_penalty = total_penalty[i];
            best_card_index = i;
        }
    }
    printf("\n");
    return best_card_index;
}