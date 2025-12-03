#include <stdio.h>

#include "User/user.h"
#include "Protocol/protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int user_handle_packet(user_t* user, const uint8_t type, const uint8_t* payload, const uint16_t payload_length) {
    switch (type) {
        case PKT_WELCOME:
            user_send_simple(user, PKT_REGISTER); break;
        case PKT_SYNC_STATE:
            printf("Sync State acquire\n"); return 0;
        default:
            printf("Получен пакет %d\n", type); return 0;
    }
    return 0;
}

void user_handle_stdin(user_t* user) {
    char buf[128];
    const ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return;

    if (strncmp(buf, "new", 3) == 0) {
        user_send_simple(user, PKT_SESSION_CREATE);
    }
    if (strncmp(buf, "quit_s", 6) == 0) {
        user_send_simple(user, PKT_SESSION_LEAVE);
    }
}
