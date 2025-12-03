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
        user->fds[0].events |= POLLOUT;
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
