//
// Created by piton on 11/12/2025.
//

#ifndef DISPLAY_SESSION_H
#define DISPLAY_SESSION_H

#include "Protocol/protocol.h"
#include "User/user.h"

void display_hand(pkt_card_t* board_cards,user_t* user,uint32_t hand_count,uint8_t allie);
void display_board(pkt_card_t* board_cards,user_t* user);
void display_scores(pkt_player_score_t* scores,uint16_t player_count);
void display_hand_teammate();
void display_card(pkt_card_t* card,user_t* user);
void display_card_score();
void up_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void mid_up_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void mid_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void void_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void mid_down_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void down_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user);
void display_extr();


#endif //DISPLAY_SESSION_H