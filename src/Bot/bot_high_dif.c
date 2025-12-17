#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "Protocol/protocol.h"
#include "User/bot.h"

uint32_t rand_state = 12345;
uint32_t fast_rand() {
    uint32_t x = rand_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rand_state = x;
}

void shuffle(int* array, const int len) {
    if (len <= 1) return;
    for (int i = len - 1; i > 0; i--) {
        const int j = (int)(fast_rand() % (i+1));
        const int tmp = array[i];
        array[i] = array[j];
        array[j] = tmp;
    }
}

int simple_enemy_choice(const int* last_in_row, const int* cards_in_row, const int row_number, const int row_length, const int* hand_cards, const int hand_length) {
    int best_card_index = -1;
    int min_diff = UINT16_MAX;
    for (int i = 0; i < hand_length; i++) {
        const int card = hand_cards[i];
        if (card == 0) continue;
        for (int row = 0; row < row_number; row++) {
            if (cards_in_row[row] == row_length) {best_card_index = -2; continue;}
            const int diff = card - last_in_row[row];
            if (diff > 0 && diff < min_diff) {
                min_diff = diff;
                best_card_index = i;
            }
        }
    }

    if (best_card_index >= 0) {
        const int card_to_return = hand_cards[best_card_index];
        return card_to_return;
    }
    if (best_card_index == -2) {
        int max_card_index = 0;
        for (int i = 0; i < hand_length; i++) {
            if (hand_cards[i] != 0 && hand_cards[i] > hand_cards[max_card_index]) max_card_index = i;
        }
        return hand_cards[max_card_index];
    }

    int min_card_index = 0;
    for (int i = 0; i < hand_length; i++) {
        if (hand_cards[i] != 0 && hand_cards[i] < hand_cards[min_card_index]) min_card_index = i;
    }
    return hand_cards[min_card_index];
}

int simulation_step(int* last_in_row, int* number_of_cards_in_row, int* row_penalty, const int row_number, const int row_length,
    int* played_cards, const int number_of_players) {

    // printf("Row length: %d\n", row_length);
    // for (int r = 0; r < row_number; r++) {
    //     printf("Row %d: (%d, %d, %d)\n", r, last_in_row[r], number_of_cards_in_row[r], row_penalty[r]);
    // } printf("Played card: ");
    // for (int p = 0; p < number_of_players; p++) {
    //     printf("%d, ", played_cards[p]);
    // } printf("\n");

    int penalty = 0;
    for (int i = 0; i < number_of_players; i++) {
        int min_card_index = -1;
        for (int p = 0; p < number_of_players; p++) {
            if (played_cards[p] == 0) continue;
            if (min_card_index == -1 || played_cards[p] < played_cards[min_card_index]) {
                min_card_index = p;
            }
        }

        int optimal_row = -1;
        int min_diff = UINT16_MAX;
        for (int row = 0; row < row_number; row++) {
            if (last_in_row[row] > played_cards[min_card_index]) continue;
            const int diff = played_cards[min_card_index] - last_in_row[row];
            if (diff < min_diff) {
                min_diff = diff;
                optimal_row = row;
            }
        }

        if (optimal_row == -1) {
            int optimal_row_to_take = 0;
            int min_penalty = row_penalty[0];
            for (int row = 1; row < row_number; row++) {
                if (row_penalty[row] < min_penalty) {
                    optimal_row_to_take = row;
                    min_penalty = row_penalty[row];
                }
            }
            if (min_card_index == 0) penalty += min_penalty;
            last_in_row[optimal_row_to_take] = played_cards[min_card_index];
            number_of_cards_in_row[optimal_row_to_take] = 1;
            row_penalty[optimal_row_to_take] = 3;
        } else {
            if (number_of_cards_in_row[optimal_row] == row_length) {
                if (min_card_index == 0) penalty += row_penalty[optimal_row];
                last_in_row[optimal_row] = played_cards[min_card_index];
                number_of_cards_in_row[optimal_row] = 1;
                row_penalty[optimal_row] = 2;
            } else {
                last_in_row[optimal_row] = played_cards[min_card_index];
                number_of_cards_in_row[optimal_row]++;
                row_penalty[optimal_row] += 3;
            }
        }
        played_cards[min_card_index] = 0;
    }
    return penalty;
}

int best_card_choice(const int* placed_cards,
                     const pkt_card_t* my_hand, const int number_of_cards_in_hand, const int number_of_players,
                     const pkt_card_t* board, const int row_number, const int row_length, const int max_value) {
    int total_penalty[number_of_cards_in_hand];
    memset(total_penalty, 0, number_of_cards_in_hand * sizeof(int));
    const int simulation = 1000;

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

    int unknown_cards[unknow_card_count];
    int index = 0;
    for (int c = 0; c < max_value; c++) {
        if (all_cards[c] == 1) continue;
        unknown_cards[index] = c+1;
        index++;
    }

    int card_choices[number_of_players];
    for (int sim = 0; sim < simulation; sim++) {
        shuffle(unknown_cards, unknow_card_count);
        for (int player = 1; player < number_of_players; player++) {
            card_choices[player] = simple_enemy_choice(last_in_row, number_of_cards_in_row,
                row_number, row_length, unknown_cards + (player - 1)* number_of_cards_in_hand, number_of_cards_in_hand);
        }
        for (int i = 0; i < number_of_cards_in_hand; i++) {
            card_choices[0] = hand[i];

            int tmp_last_in_row[row_number]; memcpy(tmp_last_in_row, last_in_row, row_number * sizeof(int));
            int tmp_row_penalty[row_number]; memcpy(tmp_row_penalty, row_penalty, row_number * sizeof(int));
            int tmp_number_of_cards_in_row[row_number]; memcpy(tmp_number_of_cards_in_row, number_of_cards_in_row, row_number * sizeof(int));
            int tmp_card_choices[number_of_players]; memcpy(tmp_card_choices, card_choices, number_of_players * sizeof(int));

            total_penalty[i] += simulation_step(tmp_last_in_row, tmp_number_of_cards_in_row, tmp_row_penalty, row_number, row_length, tmp_card_choices, number_of_players);

        }
    }

    int min_penalty = total_penalty[0];
    int best_card_index = 0;
    // printf("My cards:");
    for (int i = 0; i < number_of_cards_in_hand; i++) {
        if (total_penalty[i] < min_penalty) {
            min_penalty = total_penalty[i];
            best_card_index = i;
        }
        // printf(" ,(%d, %d)", my_hand[i].num, total_penalty[i]);
    }
    // printf("\n");
    return best_card_index;
}