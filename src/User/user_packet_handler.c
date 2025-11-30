#include <stdio.h>

#include "User/user.h"
#include "Protocol/protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int user_handle_packet(user_t* user) {
    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;
    int packet_size = 0;

    switch (user->packet_type) {
        case PKT_WELCOME:
            packet_size = create_simple_packet(buffer, PKT_REGISTER, user->session_id, user->user_id);
            break;
        case PKT_SYNC_STATE:
            printf("Sync State acquire\n"); free(buffer); return 0;
        default:
            free(buffer);
            printf("Получен пакет %d\n", user->packet_type); return 0;
    }

    user_enqueue_message(user, buffer, packet_size);
    free(buffer);
    return 0;
}


void user_handle_stdin(user_t* user) {
    char buf[128];
    const ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return;

    if (strncmp(buf, "new", 3) == 0) {
        header_t h;
        protocol_header_encode(&h, PKT_SESSION_CREATE, user->user_id, user->session_id, sizeof(header_t));
        user_enqueue_message(user, (const uint8_t*)&h, sizeof(h));
    }
    if (strncmp(buf, "quit_s", 6) == 0) {
        header_t h;
        protocol_header_encode(&h, PKT_SESSION_LEAVE, user->user_id, user->session_id, sizeof(header_t));
        user_enqueue_message(user, (const uint8_t*)&h, sizeof(h));
    }
}
