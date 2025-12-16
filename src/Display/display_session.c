//
// Created by piton on 11/12/2025.
//

#include <stdio.h>
#include <stdint.h>
#include <locale.h>
#include  "Display/display_session.h"
#include "User/user.h"

void display_hand(pkt_card_t* board_cards,user_t* user,const uint32_t hand_count,uint8_t allie) {
    setlocale(LC_ALL, "");

    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    for (uint32_t i=0;i<hand_count;i++) {
        if (hand_count/2 == i && !allie) {
            printf("─────MY_DECK────");
        }
        else if (hand_count/2 == i && allie) {
            printf("─────DECK_ALLIE────");
        }
        else {
            printf("────────────────");
        }
    }
    printf("\n");

    up_display_card(hand_count,board_cards,0,user);
    mid_up_display_card(hand_count,board_cards,0,user);
    void_display_card(hand_count,board_cards,0,user);
    mid_display_card(hand_count,board_cards,0,user);
    void_display_card(hand_count,board_cards,0,user);
    mid_down_display_card(hand_count,board_cards,0,user);
    down_display_card(hand_count,board_cards,0,user);
    printf("%s", reset);
    printf("\n");
    couleur = "\033[36m";
    printf("%s", couleur);
    printf(" 🠢 Tu peux jouer une carte de ton deck avec la commande 'play numerodecarte' ou numerodecarte est entre 0 à %u !", hand_count-1);
    printf("%s", reset);
    printf("\n");
}


void display_scores(pkt_player_score_t* scores,uint16_t player_count){
    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    for (uint32_t i=0;i<player_count;i++) {
        if (player_count/2 == i) {
            printf("─────SCORES────");
        }
        else {
            printf("────────────────");
        }
    }
    printf("%s", reset);
    printf("\n");

    couleur = "\033[36m";
    printf("%s", couleur);
    for (int i =0 ; i<player_count;i++) {
        printf(" 🠢 L'identifiant %u à %u têtes !", scores[i].player_id, scores[i].nb_head);
    }
    printf("%s", reset);
    printf("\n");
}


void display_board(pkt_card_t* board_cards,user_t* user) {

    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    for (int i=0;i<user->nb_card_line;i++) {
        if (user->nb_card_line/2 == i ) {
            printf("──────BOARD─────");
        }
        else {
            printf("──────────────────");
        }
    }
    printf("\n");
    printf("%s", reset);

    for (uint16_t r = 0; r < user->nb_line; ++r) {
            setlocale(LC_ALL, "");
            up_display_card(user->nb_card_line,board_cards,r,user);
            mid_up_display_card(user->nb_card_line,board_cards,r,user);
            void_display_card(user->nb_card_line,board_cards,r,user);
            mid_display_card(user->nb_card_line,board_cards,r,user);
            void_display_card(user->nb_card_line,board_cards,r,user);
            mid_down_display_card(user->nb_card_line,board_cards,r,user);
            down_display_card(user->nb_card_line,board_cards,r,user);
        printf("\n");
    }

    printf("\n");

}

void up_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {

    char *couleur = "\033[1;33m";
    char *reset = "\033[0m";
    printf("%s", couleur);
    printf("\n");
    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            //couleur = "\033[34m";
            continue;
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" ╭────────────╮ ");
    }
    printf("%s", reset);
}

void mid_up_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {
    printf("\n");

    char *reset = "\033[0m";
    char *couleur;

    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            //couleur = "\033[34m";
            continue;
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" │ %-3d        │ ", board_cards[indiceLigne * user->nb_card_line + i].num);
    }
    printf("%s", reset);
}
void void_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {
    printf("\n");
    char *couleur = "\033[1;33m";
    char *reset = "\033[0m";
    printf("%s", couleur);
    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            //couleur = "\033[34m";
            continue;
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" │            │ ");
    }
    printf("%s", reset);
}
void mid_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {
    printf("\n");
    char *couleur = "\033[1;33m";
    char *reset = "\033[0m";
    printf("%s", couleur);
    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            //couleur = "\033[34m";
            continue;
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" │   %3d🐮    │ ", board_cards[indiceLigne * user->nb_card_line + i].numberHead);
    }
    printf("%s", reset);
}
void mid_down_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {
    printf("\n");
    char *couleur = "\033[1;33m";
    char *reset = "\033[0m";
    printf("%s", couleur);
    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            //couleur = "\033[34m";
            continue;
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" │        %3d │ ",  board_cards[indiceLigne * user->nb_card_line + i].num);
    }
    printf("%s", reset);
}

void down_display_card(uint8_t nbrCards,pkt_card_t* board_cards,uint8_t indiceLigne,user_t* user) {
    char *couleur = "\033[1;33m";
    char *reset = "\033[0m";
    printf("%s", couleur);
    printf("\n");
    for (int i =0; i<nbrCards;i++) {
        if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 0) {
            couleur = "\033[34m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead < 3) {
            couleur = "\033[33m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 3) {
            couleur = "\033[38;5;208m";
        }
        else if (board_cards[indiceLigne * user->nb_card_line + i].numberHead == 4) {
            couleur = "\033[1;31m";
        }
        else {
            couleur = "\033[35m";
        }
        printf("%s", couleur);
        printf(" ╰────────────╯ ");
    }
    printf("%s", reset);

}
void display_extr() {
    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    printf("──────────────────────EXTR───────────────────────");

    printf("\n");
    printf("%s", reset);

    couleur = "\033[36m";
    printf("%s", couleur);
    printf(" 🠢 6 QUI PRENDS ! Choisi maintenant la ligne que tu veux extraire avec la commande 'extr numérodeligne' !");
    printf("%s", reset);
    printf("\n");
}

void display_start() {
    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    printf("──────────────────────WELCOME───────────────────────");

    printf("\n");
    printf("%s", reset);

    couleur = "\033[36m";
    printf("%s", couleur);
    printf("Bienvenue dans notre jeu 6 QUI PRENDS ! \n");
    printf("Pour commencer, voici les commandes qui sont à ta disposition pour pouvoir : \n");
    printf("🠢 Crée une session : 'new nbrPlayer Visible' \n");
    printf("Ou nbrPlayer(int) est le nombre de joueurs max dans la session et Visible(0 ou 1) pour indiquer si la session est visible  \n");
    printf("🠢 Liste des sessions : 'slist' \n");
    printf("🠢 Rejoindre une session : 'join id_session' \n");
    printf("Ou id_session(int) est l'identifiant de la session que vous voulez rejoindre\n");
    printf("🠢 Quitter  : 'unreg' \n");
    printf("%s", reset);
    printf("\n");
}

void display_debug_comm() {
    printf("new - new_session\nadd_bot - add_bot\nquit_s - quit session\nunreg - unregister\n");
    printf("dconn - disconnect\nrconn - reconnect\nslist - session list\nstart - session start\n");
    printf("close - sessoin close\ndebug - on/off debug\n join - join session\nplay - make move\n extr - extra\n");
}

void display_in_session() {
    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    printf("%s", couleur);
    printf("──────────────────────IN_SESSION───────────────────────");

    printf("\n");
    printf("%s", reset);

    couleur = "\033[36m";
    printf("%s", couleur);
    printf("Tu as crée une session ! Tu peux maintenant effectuer ses commandes pour : \n");
    printf("🠢 Lancer la partie (attention à attendre des joueurs ou ajouter des bots pour ne pas être seul): 'start' \n");
    printf("🠢 Ajouter un bot : 'add_bot nbrBot difficulter_du_bot' \n");
    printf("Ou nbrBot(int) est le nombre de bot et difficulter_du_bot(0,1 ou 2) avec 0 -> débutant , 1 -> Intermédiare , 2 -> Expert  \n");
    printf("🠢 Quitter la session : 'quit_s' \n");
    printf("%s", reset);
    printf("\n");
}
void display_phase_result(pkt_player_score_t* scores,uint16_t player_count) {
    char *couleur = "\033[32m";
    char *reset = "\033[0m";

    int perdant = -1;
    int id_perdant = -1;
    int gagnant = 10000;
    int id_gagnant = -1;

    printf("%s", couleur);
    printf("──────────────────────PHASE_RESULT───────────────────────");

    printf("\n");
    printf("%s", reset);

    couleur = "\033[36m";
    printf("%s", couleur);
    printf("Fin de session ! Voici les scores finaux : \n");
    couleur = "\033[36m";
    printf("%s", couleur);
    for (int i =0 ; i<player_count;i++) {
        if (scores[i].nb_head > perdant) {
            perdant = scores[i].nb_head;
            id_perdant = i;
        }
        if (scores[i].nb_head < gagnant) {
            gagnant = scores[i].nb_head;
            id_gagnant = i;
        }
        printf(" 🠢 L'identifiant %u à %u têtes ! \n", scores[i].player_id, scores[i].nb_head);
    }
    printf("Notre gagnant est %u avec un total de %u \n",scores[id_gagnant].player_id ,scores[id_gagnant].nb_head);
    printf("Notre perdant est %u avec un total de %u \n",scores[id_perdant].player_id ,scores[id_perdant].nb_head);
    printf("%s", reset);
    printf("\n");
}




