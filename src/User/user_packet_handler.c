#include <stdio.h>

#include "User/user.h"
#include "Protocol/protocol.h"
#include "Display/display_session.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

int user_handle_packet(user_t* user, const uint8_t type, const uint8_t* payload, const uint16_t payload_length) {
    switch (type) {
        case PKT_WELCOME:
            if (user->client_id == 0) {
                user_send_simple(user, PKT_REGISTER);
            }
            else user_send_reconnect(user); // PKT_RECONNECT
            return 0;
        case PKT_SYNC_STATE:
            client_handle_sync_state(user, payload, payload_length);
            display_start();
            return 0;
        case PKT_SESSION_LIST:
            return client_handle_session_list(payload, payload_length);
        case PKT_SESSION_STATE:
            client_handle_session_state(user, payload, payload_length);
            display_in_session();
            return 0;
        case PKT_SESSION_INFO:
            return client_handle_session_info(user, payload, payload_length);
        case PKT_REQUEST_EXTRA:
            return client_handle_request_extra(user, payload, payload_length);
        case PKT_SESSION_END:
            return user_quit_session(user);
        case PKT_PHASE_RESULT:
            client_handle_phase_result(user, payload, payload_length);
            return 0;
        default:
            printf("Got packet %d\n", type); return 0;
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


static const char* skip_ws(const char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static int parse_int(const char* s, int* out, const char** endp) {
    s = skip_ws(s);
    errno = 0;
    char* end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s) return 0;
    if (errno == ERANGE || v < INT8_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    if (endp) *endp = end;
    return 1;
}

void user_execute_command(user_t* user, const char* cmd) {
    if (!user || !cmd) return;

    cmd = skip_ws(cmd);
    if (*cmd == '\0') return;

    char op[32];
    size_t i = 0;
    while (cmd[i] && !isspace((unsigned char)cmd[i]) && i + 1 < sizeof(op)) {
        op[i] = cmd[i];
        i++;
    }
    op[i] = '\0';

    const char* args = cmd + i;

    if (strcmp(op, "new") == 0) {
        int number_of_player = 4;
        int is_visible = 1;

        const char* p = args;
        int tmp;
        if (parse_int(p, &tmp, &p)) number_of_player = tmp;
        if (parse_int(p, &tmp, &p)) is_visible = tmp;

        if (number_of_player <= 0) {
            fprintf(stderr, "Bad creation of session. Usage: new <player number> <visibility>\n");
            return;
        }
        user_send_create_session(user, number_of_player, is_visible);
        return;
    }

    if (strcmp(op, "add_bot") == 0) {
        int number_of_bots = 1;
        int bot_difficulty = 1;

        const char* p = args;
        int tmp;
        if (parse_int(p, &tmp, &p)) number_of_bots = tmp;
        if (parse_int(p, &tmp, &p)) bot_difficulty = tmp;

        if (number_of_bots <= 0) {
            fprintf(stderr, "Bad creation of bot. Usage: add_bot <number> <difficulty>\n");
            return;
        }
        user_send_add_bot(user, number_of_bots, bot_difficulty);
        return;
    }

    if (strcmp(op, "quit_s") == 0) { user_send_simple(user, PKT_SESSION_LEAVE); return;}
    if (strcmp(op, "unreg") == 0) { user_send_simple(user, PKT_UNREGISTER); return;}
    if (strcmp(op, "dconn") == 0) { user_close_connection(user); return;}
    if (strcmp(op, "rconn") == 0) { user_send_reconnect(user); return;}
    if (strcmp(op, "slist") == 0) { user_send_simple(user, PKT_GET_SESSION_LIST); return;}
    if (strcmp(op, "start") == 0) { user_send_simple(user, PKT_START_SESSION); return;}
    if (strcmp(op, "debug") == 0) { user->debug_mode = 1;}

    if (strcmp(op, "join") == 0) {
        int session_id;
        if (parse_int(args, &session_id, NULL)) user_send_join_session(user, session_id);
        else fprintf(stderr, "Bad join command. Usage: join <number>\n");
        return;
    }

    if (strcmp(op, "play") == 0) {
        int card_value;
        if (parse_int(args, &card_value, NULL)) user_send_info_return(user, card_value);
        else fprintf(stderr, "Bad play command. Usage: play <number>\n");
        return;
    }

    if (strcmp(op, "extr") == 0) {
        int row_number;
        if (parse_int(args, &row_number, NULL)) user_send_response_extra(user, row_number);
        else fprintf(stderr, "Bad extr command. Usage: extr <number>\n");
        return;
    }

    fprintf(stderr, "Unknown command: %s\n", op);
}
