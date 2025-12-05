#include <stdio.h>

#include "User/user.h"
#include "Protocol/protocol.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int user_handle_packet(user_t* user, const uint8_t type, const uint8_t* payload, const uint16_t payload_length) {
    switch (type) {
        case PKT_WELCOME:
            if (user->client_id == 0) {
                user_send_simple(user, PKT_REGISTER);
            }
            // else user_send_reconnect(user); // PKT_RECONNECT
            return 0;
        case PKT_SYNC_STATE:
            return client_handle_sync_state(user, payload, payload_length);
        case PKT_SESSION_LIST:
            return client_handle_session_list(payload, payload_length);
        default:
            printf("Получен пакет %d\n", type); return 0;
    }
}

void user_handle_stdin(user_t* user) {
    char buf[128];
    const ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));

    if (n > 0) {
        buf[n] = 0;
        user_execute_command(user, buf);
    }
}


void user_execute_command(user_t* user, const char* cmd) {
    if (strncmp(cmd, "new", 3) == 0) user_send_simple(user, PKT_SESSION_CREATE);
    else if (strncmp(cmd, "quit_s", 6) == 0) user_send_simple(user, PKT_SESSION_LEAVE);
    else if (strncmp(cmd, "unreg", 5) == 0) user_send_simple(user, PKT_UNREGISTER);
    else if (strncmp(cmd, "dconn", 5) == 0) user_close_connection(user);
    else if (strncmp(cmd, "rconn", 5) == 0) user_send_reconnect(user);
    else if (strncmp(cmd, "slist", 5) == 0) user_send_simple(user, PKT_GET_SESSION_LIST);
    else if (strncmp(cmd, "join1", 5) == 0) user_send_join_session(user, 1);
}
