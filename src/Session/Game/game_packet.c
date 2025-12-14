#include <stdio.h>
#include <stdlib.h>
#include <sys/random.h>

#include "log_bus.h"
#include "Session/Game/game.h"
#include "Protocol/protocol.h"
#include "Session/session.h"

int get_real_index_from_visible(const int* cards, const int len, const int visible_idx) {
    int v = 0;
    for (int i = 0; i < len; ++i) {
        if (cards[i] == -1) continue;
        if (v == visible_idx) return i;
        v++;
    }
    return -1; // no such card
}

int start_move(const session_t* session) {
    if (session->game->game_state != WAITING && session->game->game_state != WAITING_EXTRA) return 0;
    if (session->game->card_ready_to_place_count >= session->number_players && session->game->card_ready_to_place_count > 0) {
        session->game->game_state = PLAYING;
        push_modification_log(session,ACTION);
        for (int i = 0; i < session->capacity; ++i) {
            if (session->players[i].player_id != 0) continue;
            session->game->card_ready_to_place[i] = -1;
        }
        return place_card(session->game, session->players, session->capacity);
    }
    return 0;
}

int game_handle_info_return(session_t* session, const session_message_t* msg) {
    if (session->game->game_state != WAITING) {
        log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Phase id not Waiting it is %d", session->game->game_state);
        return -1;
    }
    if (msg->data.user.payload_length < sizeof(pkt_session_info_return_payload_t)) {
        log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Bad session info return payload");
        session_send_error_packet(session, msg->data.user.client_id, 0x34, "Bad session info return payload\n");
        return -1;
    }
    const pkt_session_info_return_payload_t* pkt = (const pkt_session_info_return_payload_t*)msg->data.user.buf;
    const uint8_t player_index = get_index_by_id(session, msg->data.user.client_id);
    if (session->game->card_ready_to_place[player_index] != -1) {

        log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Card_ready to place for this player is %d", session->game->card_ready_to_place[player_index]);
        return -1;
    }

    const player_t* player = &session->players[player_index];
    const int real_card_index = get_real_index_from_visible(player->player_cards_id, session->game->nbrCardsPlayer, pkt->card_id);
    if (real_card_index == -1) {log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Got index < %d for index %d (player_id: %d)",real_card_index, pkt->card_id, msg->data.user.client_id);return -1;}
    if (player->player_cards_id[real_card_index] == -1) {log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Player has already played card with real index %d",real_card_index); return -1;}

    const int card_id = player->player_cards_id[real_card_index];
    player->player_cards_id[real_card_index] = -1;

    session->game->deck[card_id].client_id = msg->data.user.client_id;
    session->game->card_ready_to_place[player_index] = card_id;
    session->game->card_ready_to_place_count += 1;
    return 0;
}

int game_handle_response_extra(session_t* session, const session_message_t* msg) {
    if (session->game->game_state != WAITING_EXTRA) return -1;
    if (msg->data.user.client_id != session->game->extra_wait_from) {
        log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Unauthorized access");
        session_send_error_packet(session, msg->data.user.client_id, 0x01, "Unauthorized access");
        return -1;
    }
    if (msg->data.user.payload_length < sizeof(pkt_response_extra_payload_t)) {
        log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Bad session response extra payload");
        session_send_error_packet(session, msg->data.user.client_id, 0x34, "Bad session response extra payload");
        return -1;
    }
    const pkt_response_extra_payload_t* pkt = (const pkt_response_extra_payload_t*)msg->data.user.buf;
    const int16_t row_index = pkt->row_index;

    const int index_card_to_place = get_index_by_id(session, msg->data.user.client_id);
    push_modification_log(session,EXTR);
    take_line(session->game, session->players, row_index, index_card_to_place);
    return place_card(session->game, session->players, session->capacity);
}

int game_send_request_extra(session_t* session, const uint32_t user_id) {
    const game_t* game = session->game;
    uint8_t card_count = 0;
    for (size_t i = 0; i < session->capacity; ++i) {
        const int card_index = session->game->card_ready_to_place[i];
        if (card_index != -1) {
            ++card_count;
        }
    }

    if (card_count == 0) return -1;

    const size_t header_part = sizeof(pkt_request_extra_payload_t);
    const size_t cards_part = (size_t)card_count * sizeof(pkt_card_t);
    const size_t payload_length = header_part + cards_part;

    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* payload = malloc(payload_length);

    if (!payload) {
        log_bus_push_message(session->log_bus,session->id,LOG_ERROR,"Allocation Error");
        return -1;
    }

    pkt_request_extra_payload_t* pkt = (pkt_request_extra_payload_t*)payload;
    pkt->card_count = card_count;

    pkt_card_t* out_card = (pkt_card_t*)(pkt + 1);

    uint8_t index = 0;
    for (size_t i = 0; i < session->capacity; ++i) {
        const int card_index = session->game->card_ready_to_place[i];
        if (card_index == -1) continue;

        const card_t* card = &game->deck[card_index];
        out_card[index].num = card->num;
        out_card[index].numberHead = card->numberHead;
        out_card[index].client_id = card->client_id;
        ++index;
    }
    const int result = session_send_to_player(session, user_id,PKT_REQUEST_EXTRA, payload, payload_length);
    if (result != -1) {
        session->game->extra_wait_from = user_id;
    }
    free(payload);
    return result;
}
