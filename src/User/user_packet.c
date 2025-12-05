#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "User/user.h"

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
    printf("Token %s\n", user->reconnection_token);
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
    for (uint16_t i = 0; i < count; i++) {
        const uint32_t session_id = ids[i];
        printf("%d, ", session_id);
    }
    printf(" \n");
    return 0;
}