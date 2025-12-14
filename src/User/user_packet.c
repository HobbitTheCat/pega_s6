#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "User/user.h"
#include "Display/display_session.h"

int user_enqueue_message(user_t* user, const uint8_t* buffer, const uint32_t total_length) {
    tx_state_t* tx = &user->tx;
    if (tx->queued + total_length > tx->high_watermark) return -1;

    out_chunk_t* chunk = malloc(sizeof(out_chunk_t));
    if (!chunk) return -1;

    chunk->data = malloc(total_length);
    if (!chunk->data) {free(chunk); return -1;}
    memcpy(chunk->data, buffer, total_length);
    chunk->len = total_length;
    chunk->off = 0;
    chunk->next = NULL;

    if (!tx->tail) tx->head = tx->tail = chunk;
    else {tx->tail->next = chunk; tx->tail = chunk;}

    tx->queued += total_length;
    user_handle_write(user);
    if (tx->head && user->want_pollout == 0) {
        user->fds[1].events |= POLLOUT;
        user->want_pollout = 1;
    }
    return 0;
}

int user_send_packet(user_t* user, uint8_t type, const void* payload, const uint16_t payload_length) {
    const uint32_t total_length = sizeof(header_t) + payload_length;
    if (total_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* buff = malloc(total_length);
    if (!buff) return -1;

    header_t* header = (header_t*)buff;
    if (protocol_header_encode(header, type, payload_length) < 0) {free(buff); return -1;}

    if (payload_length > 0 && payload) memcpy(buff + sizeof(header_t), payload, payload_length);
    const int result = user_enqueue_message(user, buff, total_length);
    free(buff);
    return result;
}

int user_send_simple(user_t* user, const uint8_t type) {
    return user_send_packet(user, type, NULL, 0);
}

int user_send_reconnect(user_t* user) {
    const char* token = user->reconnection_token;
    const size_t token_length = strlen(token);
    const size_t header_part = sizeof(pkt_reconnect_payload_t);
    const size_t payload_length = token_length + header_part;
    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;
    pkt_reconnect_payload_t* packet = (pkt_reconnect_payload_t*)payload;
    packet->user_id = user->client_id;
    packet->token_length = token_length;
    if (token_length > 0) memcpy((uint8_t*)(packet + 1), token, token_length);
    const int result = user_send_packet(user, PKT_RECONNECT, payload, payload_length);
    free(payload);
    return result;
}

int user_send_create_session(user_t* user, const uint8_t number_players, const uint8_t is_visible) {
    const size_t payload_length = sizeof(pkt_session_create_payload_t);
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_create_payload_t* packet = (pkt_session_create_payload_t*)payload;
    packet->is_visible = is_visible;
    packet->nb_players = number_players;

    const int result = user_send_packet(user, PKT_SESSION_CREATE, payload, payload_length);

    free(payload);
    return result;
}

int user_send_join_session(user_t* user, const uint32_t session_id) {
    const size_t payload_length = sizeof(pkt_session_join_payload_t);
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_join_payload_t* packet = (pkt_session_join_payload_t*)payload;
    packet->session_id = session_id;

    const int result = user_send_packet(user, PKT_SESSION_JOIN, payload, payload_length);
    free(payload);
    return result;
}

int user_send_set_session_rules(user_t* user, const uint8_t is_visible, const uint8_t nb_lines,
        const uint8_t nb_cards_line, const uint8_t nb_cards, const uint8_t nb_cards_player, const uint8_t max_heads) {
    const size_t payload_length = sizeof(pkt_session_set_game_rules_payload_t);
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_set_game_rules_payload_t* packet = (pkt_session_set_game_rules_payload_t*)payload;
    packet->is_visible = is_visible;
    packet->nb_lines = nb_lines;
    packet->nb_cards_line = nb_cards_line;
    packet->nb_cards = nb_cards;
    packet->nb_cards_player = nb_cards_player;
    packet->max_heads = max_heads;
    const int result = user_send_packet(user, PKT_SESSION_SET_GAME_RULES, payload, payload_length);
    free(payload);
    return result;
}

int user_send_add_bot(user_t* user, const uint8_t number_of_bot_to_add, const uint8_t bot_difficulty) {
    const size_t payload_length = sizeof(pkt_bot_add_payload_t);
    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_bot_add_payload_t* pkt = (pkt_bot_add_payload_t*)payload;
    pkt->number_of_bot_to_add = number_of_bot_to_add;
    pkt->bot_difficulty = bot_difficulty;

    const int result = user_send_packet(user, PKT_ADD_BOT, payload, payload_length);
    free(payload);
    return result;
}


int user_send_info_return(user_t* user, const uint8_t card_choice) {
    const size_t payload_length = sizeof(pkt_session_info_return_payload_t);
    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_session_info_return_payload_t* packet = (pkt_session_info_return_payload_t*)payload;
    packet->card_id = card_choice;
    const int result = user_send_packet(user, PKT_SESSION_INFO_RETURN, payload, payload_length);
    free(payload);
    return result;
}

int user_send_response_extra(user_t* user, const int16_t row_index) {
    const size_t payload_length = sizeof(pkt_response_extra_payload_t);
    if (sizeof(header_t) + payload_length > PROTOCOL_MAX_SIZE) return -1;

    uint8_t* payload = malloc(payload_length);
    if (!payload) return -1;

    pkt_response_extra_payload_t* pkt = (pkt_response_extra_payload_t*)payload;

    pkt->row_index = row_index;
    const int result = user_send_packet(user, PKT_RESPONSE_EXTRA, payload, payload_length);

    free(payload);
    return result;
}

int client_handle_request_extra(user_t* user,
                                const uint8_t* payload,
                                const uint16_t payload_length)
{
    if (!user->game_initialized) {
        fprintf(stderr, "Client: REQUEST_EXTRA received before SESSION_STATE\n");
        return -1;
    }

    if (payload_length < sizeof(pkt_request_extra_payload_t)) {
        fprintf(stderr, "Client: bad REQUEST_EXTRA length\n");
        return -1;
    }

    const pkt_request_extra_payload_t* hdr =
        (const pkt_request_extra_payload_t*)payload;

    const uint8_t card_count = hdr->card_count;

    const size_t expected =
        sizeof(pkt_request_extra_payload_t) +
        (size_t)card_count * sizeof(pkt_card_t);

    if (payload_length < expected) {
        fprintf(stderr, "Client: truncated REQUEST_EXTRA cards\n");
        return -1;
    }

    pkt_card_t* cards = (pkt_card_t*)(hdr + 1);

    display_extr(cards,card_count,user);

    return 0;
}



int client_handle_sync_state(user_t* user, const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_sync_state_payload_t)) {
        fprintf(stderr, "Client: bad SYNC_STATE length\n"); return -1;
    }
    const pkt_sync_state_payload_t* sync_state = (pkt_sync_state_payload_t*) payload;
    const uint16_t token_length = sync_state->token_length;
    const size_t header_part = sizeof(pkt_sync_state_payload_t);
    if (payload_length < header_part + token_length) {
        fprintf(stderr, "Client: truncated SYNC_STATE token\n"); return -1;
    }
    const uint8_t* token_ptr = (const uint8_t*)(sync_state + 1);
    user->client_id = sync_state->user_id;
    if (token_length >= sizeof(user->reconnection_token)) {
        fprintf(stderr, "Client: reconnect token too long\n"); return -1;
    }
    memcpy(user->reconnection_token, token_ptr, token_length);
    user->reconnection_token[token_length] = '\0';

    return 0;
}

int client_handle_session_list(const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_session_list_payload_t)) {
        fprintf(stderr, "Client: bad SESSION_LIST length\n"); return -1;
    }
    const pkt_session_list_payload_t* session_list = (pkt_session_list_payload_t*) payload;
    const uint16_t count = session_list->count;
    const size_t expected = sizeof(pkt_session_list_payload_t) + (size_t)count * sizeof(uint32_t);
    if (payload_length < expected) {
        fprintf(stderr, "Client: bad SESSION_LIST length\n"); return -1;
    }
    const uint32_t* ids = (const uint32_t*) (session_list + 1);
    printf("Session ids: ");
    if (count == 0) {printf("\n"); return 0;}
    printf("%d", ids[0]);
    for (uint16_t i = 1; i < count; i++) {
        const uint32_t session_id = ids[i];
        printf(", %d", session_id);
    }
    printf(" \n");
    return 0;
}

int client_handle_session_state(user_t* user, const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_session_state_payload_t)) {
        fprintf(stderr, "Client: bad SESSION_STATE length\n");
        return -1;
    }

    const pkt_session_state_payload_t* pkt = (pkt_session_state_payload_t*) payload;
    user_join_session(user, pkt->max_card_value, pkt->nbrLign, pkt->nbrCardsLign, pkt->nbrCardsPlayer);
    printf("Session id: %d\n", pkt->session_id);
    return 0;
}



int client_handle_session_info(user_t* user, const uint8_t* payload, const uint16_t payload_length) {
    if (!user->game_initialized) {
        fprintf(stderr, "Client: session state not received yet\n");
        return -1;
    }

    if (payload_length < sizeof(pkt_session_info_payload_t)) {
        fprintf(stderr, "Client: bad SESSION_INFO length\n");
        return -1;
    }

    const pkt_session_info_payload_t* hdr = (pkt_session_info_payload_t*)payload;
    const uint16_t hand_count = hdr->hand_count;
    const uint16_t player_count = hdr->player_count;
    const uint16_t board_length = user->nb_line * user->nb_card_line;

    const size_t expected =
        sizeof(pkt_session_info_payload_t) +
        (size_t)player_count * sizeof(pkt_player_score_t) +
        (size_t)board_length * sizeof(pkt_card_t) +
        (size_t)hand_count * sizeof(pkt_card_t);

    if (payload_length < expected) {
        fprintf(stderr, "Client: truncated SESSION_INFO\n");
        return -1;
    }

    pkt_player_score_t* scores = ( pkt_player_score_t*)(hdr + 1);
    pkt_card_t* board_cards = ( pkt_card_t*)(scores + player_count);
    pkt_card_t* hand_cards  = board_cards + board_length;


    if (user->debug_mode == 0) {
        display_scores(scores,player_count);
        display_board(board_cards,user);
        display_hand(hand_cards,user,hand_count,0);
    } else {
        printf("===== BOARD =====\n");
        for (uint16_t r = 0; r < user->nb_line; ++r) {
            for (uint16_t c = 0; c < user->nb_card_line; ++c) {
                const pkt_card_t* card = &board_cards[r * user->nb_card_line + c];

                if (card->num == 0) {
                    printf("[   ] ");
                } else {
                    printf("[%3d] ", (int)card->num);
                }
            }
            printf("\n");
        }

        printf("\n===== YOUR HAND =====\n");
        int any_hand = 0;
        for (uint16_t i = 0; i < hand_count; ++i) {
            const pkt_card_t* card = &hand_cards[i];
            // if (card->num == 0) continue;
            printf("[%3d] ", (int)card->num);
            any_hand = 1;
        }
        if (!any_hand) {
            printf("(no cards)\n");
        } else {
            printf("\n");
        }
        printf("\n");
    }

    return 0;
}

int client_handle_phase_result(const user_t* user, const uint8_t* payload, const uint16_t payload_length) {
    (void)user;

    if (payload_length < sizeof(pkt_phase_result_payload_t)) {
        fprintf(stderr, "Client: bad PHASE_RESULT length\n");
        return -1;
    }

    const pkt_phase_result_payload_t* hdr = (const pkt_phase_result_payload_t*)payload;
    const uint16_t player_count = hdr->player_count;

    const size_t expected =
        sizeof(pkt_phase_result_payload_t) +
        (size_t)player_count * sizeof(pkt_player_score_t);

    if (payload_length < expected) {
        fprintf(stderr, "Client: truncated PHASE_RESULT\n");
        return -1;
    }

    pkt_player_score_t* scores = (pkt_player_score_t*)(hdr + 1);
    display_phase_result(scores,player_count);
    return 0;
}

int client_handle_error_packet(const uint8_t* payload, const uint16_t payload_length) {
    if (payload_length < sizeof(pkt_error_payload_t)) {
        fprintf(stderr, "Client: bad ERROR payload length\n");
        return -1;
    }

    const pkt_error_payload_t* pkt =
        (const pkt_error_payload_t*)payload;

    const uint16_t msg_len = pkt->message_length;
    const size_t expected_len =
        sizeof(pkt_error_payload_t) + (size_t)msg_len;

    if (payload_length < expected_len) {
        fprintf(stderr, "Client: truncated ERROR message\n");
        return -1;
    }

    const char* msg = "";
    char* tmp = NULL;

    if (msg_len > 0) {
        tmp = malloc((size_t)msg_len + 1);
        if (!tmp) return -1;

        memcpy(tmp, (const uint8_t*)(pkt + 1), msg_len);
        tmp[msg_len] = '\0';
        msg = tmp;
    }

    fprintf(stderr, "Error (%u): %s\n", pkt->error_code, msg);

    free(tmp);
    return 0;
}
