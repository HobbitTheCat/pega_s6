#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "Session/session.h"

int session_send_to_player(const session_t* session, const uint32_t user_id, const uint8_t packet_type, const void* payload, const uint16_t payload_length) {
    if (payload_length > SP_MAX_FRAME) return -1;

    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    if (!message) return -1;
    message->type = USER_MESSAGE;
    message->data.user.client_id = user_id;
    message->data.user.packet_type = packet_type;
    message->data.user.payload_length = payload_length;
    if (payload_length > 0 && payload) memcpy(message->data.user.buf, payload, payload_length);
    if (session_bus_push(&session->bus->write, message) != 0) {free(message);return -1;}
    return 0;
}

int send_system_message_to_server(const session_t* session, const uint32_t user_id, const system_message_type_t type) {
    session_message_t* message = (session_message_t*)malloc(sizeof(session_message_t));
    message->type = SYSTEM_MESSAGE;
    message->data.system.type = type;
    message->data.system.session_id = session->id;
    message->data.system.user_id = user_id;
    if (session_bus_push(&session->bus->write, message) != 0) {free(message);return -1;}
    return 0;
}

int session_send_simple_packet(const session_t* session, const uint32_t user_id, const uint8_t packet_type) {
    return session_send_to_player(session, user_id, packet_type, NULL, 0);
}

int session_send_error_packet(const session_t* session, const uint32_t user_id, const uint8_t error_code, const char* error_message) {
    if (!error_message) error_message = "";
    const size_t message_length = strlen(error_message);
    const size_t header_part = sizeof(pkt_error_payload_t);
    const size_t payload_length = header_part + message_length;
    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_error_payload_t* packet = (pkt_error_payload_t*)payload;
    packet->error_code = error_code;
    packet->message_length = (uint16_t)message_length;
    if (message_length > 0) memcpy((uint8_t*)(packet + 1), error_message, message_length);
    const int result = session_send_to_player(session, user_id, PKT_ERROR, payload, payload_length);
    free(payload);
    return result;
}

int session_send_status(const session_t* session) {
    const size_t payload_length = sizeof(pkt_session_status_payload_t);
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_status_payload_t* packet = (pkt_session_status_payload_t*)payload;
    packet->capacity = session->capacity;
    packet->player_count = session->number_players;
    const int result = session_send_to_player(session, session->id_creator, PKT_SESSION_STATUS, payload, payload_length);
    free(payload);
    return result;
}

int session_send_state(const session_t* session, const uint32_t user_id) {
    const size_t payload_length = sizeof(pkt_session_state_payload_t);

    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_state_payload_t* packet = (pkt_session_state_payload_t*)payload;
    packet->session_id = session->id;
    packet->nbrLign = session->game->nbrLign;
    packet->nbrCardsLign = session->game->nbrCardsLign;
    packet->nbrCardsPlayer = session->game->nbrCardsPlayer;

    const int result = session_send_to_player(session, user_id, PKT_SESSION_STATE, payload, payload_length);
    free(payload);
    return result;
}

int session_send_info(const session_t* session, const uint32_t user_id) {
    const game_t* game = session->game;
    const int player_idx = get_index_by_id(session, user_id);
    if (player_idx < 0) return -1;
    const player_t* player = &session->players[player_idx];

    const uint16_t board_len = (uint16_t)(game->nbrLign * game->nbrCardsLign);
    const uint8_t  max_hand  = game->nbrCardsPlayer;
    const uint16_t hand_count = calc_hand_count(player, max_hand);

    uint16_t active_player_count = 0;
    for (int i = 0; (size_t)i < session->capacity; i++) {
        if (session->players[i].player_id != 0) {
            active_player_count++;
        }
    }

    const size_t header_part = sizeof(pkt_session_info_payload_t);
    const size_t score_part  = (size_t)active_player_count * sizeof(pkt_player_score_t);
    const size_t board_part  = (size_t)board_len * sizeof(pkt_card_t);
    const size_t hand_part   = (size_t)hand_count * sizeof(pkt_card_t);

    const size_t payload_length = header_part + score_part + board_part + hand_part;
    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) {
        printf("Session info is too big\n");
        return -1;
    }

    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    // ---- header ----
    pkt_session_info_payload_t* header = (pkt_session_info_payload_t*)payload;
    header->hand_count   = hand_count;
    header->player_count = active_player_count;

    // ---- scores ----
    pkt_player_score_t* scores_out = (pkt_player_score_t*)(header + 1);
    int p_idx = 0;
    for (int i = 0; (size_t)i < session->capacity; i++) {
        const player_t* p = &session->players[i];
        if (p->player_id == 0) continue;

        scores_out[p_idx].player_id = p->player_id;
        scores_out[p_idx].nb_head   = p->nb_head;
        p_idx++;
    }
    // опционально: assert(p_idx == active_player_count);

    // ---- board ----
    pkt_card_t* out = (pkt_card_t*)(scores_out + active_player_count);

    for (uint16_t i = 0; i < board_len; i++) {
        const int board_index = game->board[i];
        if (board_index != -1) {
            const card_t* card = &game->deck[board_index];
            out[i].num        = card->num;
            out[i].numberHead = card->numberHead;
            out[i].client_id  = card->client_id;
        } else {
            out[i].num        = 0;
            out[i].numberHead = 0;
            out[i].client_id  = 0;
        }
    }

    // ---- hand ----
    uint16_t idx = 0;
    for (uint16_t i = 0; i < game->nbrCardsPlayer; i++) {
        const int card_index = player->player_cards_id[i];
        if (card_index == -1) continue;

        const card_t* card = &game->deck[card_index];

        pkt_card_t* dst = &out[board_len + idx];
        dst->num        = card->num;
        dst->numberHead = card->numberHead;
        dst->client_id  = card->client_id;
        idx++;
    }
    // опционально: assert(idx == hand_count);

    const int result = session_send_to_player(session, user_id,
                                              PKT_SESSION_INFO,
                                              payload, payload_length);
    free(payload);
    return result;
}


int session_handle_set_rules(session_t* session, session_message_t* msg) {
    if (msg->data.user.client_id != session->id_creator) {
        session_send_error_packet(session, msg->data.user.client_id, 0x01, "Unauthorized access");
        return -1;
    }
    if (msg->data.user.payload_length < sizeof(pkt_session_set_game_rules_payload_t)) {
        session_send_error_packet(session, msg->data.user.client_id, 0x34, "Bad session set rules payload");
        return -1;
    }
    if (session->game->game_state != INACTIVE) {
        session_send_error_packet(session, msg->data.user.client_id, 0x34, "Session already started");
        return -1;
    }
    const pkt_session_set_game_rules_payload_t* pkt = (const pkt_session_set_game_rules_payload_t*)msg->data.user.buf;
    uint8_t nb_lines = pkt->nb_lines;
    uint8_t nb_cards_line = pkt->nb_cards_line;
    uint8_t nb_cards_player = pkt->nb_cards_player;
    uint8_t nb_cards = pkt->nb_cards;
    uint8_t max_heads = pkt->max_heads;

    if (nb_cards < nb_cards_player * session->capacity) {
        session_send_error_packet(session, msg->data.user.client_id, 0x34, "Not enough cards");
        return -1;
    }

    cleanup_game(session->game);
    init_game(session->game, nb_lines, nb_cards_line, nb_cards_player,
        nb_cards, max_heads, session->capacity);

    for (int i = 0; (size_t)i < session->capacity; i++) {
        const player_t* player = &session->players[i];
        if (player->player_id == 0) continue;
        session_send_state(session, player->player_id);
    }

    return 0;
}