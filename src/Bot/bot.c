#include "User/bot.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int bot_init(bot_t* bot) {
    bot->session_id = 0;
    bot->level = 0;
    return user_init(&bot->user);
}

void bot_cleanup(bot_t* bot) {
    free(bot->placed_cards); // Double free
    user_cleanup(&bot->user);
}

int bot_connect(bot_t* bot, const char* host, const uint16_t port) {
    return user_connect(&bot->user, host, port);
}

int bot_start(bot_t* bot, const char* host, const uint16_t port, const uint32_t session_id, const uint8_t level) {
    bot->session_id = session_id;
    bot->level = level;
    if (bot_connect(bot, host, port) < 0) return -1;
    return bot_run(bot);
}

int bot_run(bot_t* bot) {
    user_t* user = &bot->user;

    while (user->is_running && user->sockfd >= 0) {
        const int ret = poll(user->fds, user->nfds, -1);
        if (ret < 0) {if (errno == EINTR) continue;perror("Client: poll");return -1;}

        if (user->nfds > 1 &&
            user->fds[1].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL | POLLOUT)) {

            if (user->fds[1].revents & POLLIN) {if (bot_handle_read(bot) < 0) return -1;}
            if (user->fds[1].revents & POLLOUT) {if (bot_handle_write(bot) < 0) return -1;}
            if (user->fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) { perror("Client: socket error"); return -1;}}
    }
    printf("Cleanup\n");
    bot_cleanup(bot);
    return 0;
}

int bot_handle_read(bot_t* bot) {
    user_t* user = &bot->user;
    rx_state_t* rx = &user->rx;
    for (;;) {
        while (rx->have < sizeof(rx->buf)) {
            const ssize_t n = recv(user->sockfd, rx->buf + rx->have, sizeof(rx->buf) - rx->have, 0);
            if (n > 0) {rx->have += (uint32_t)n; continue;}
            if (n == 0) {printf("Client: server closed connection\n"); return -1;}
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("Client: recv"); return -1;
        }

        for (;;) {
            header_t header;
            uint8_t* payload;
            uint16_t payload_len;

            const int r = rx_try_extract_frame(rx, &header, &payload, &payload_len);
            if (r == 0) break;
            if (r == -1) {fprintf(stderr, "Client: bad packet from server\n"); return -1;}
            bot_handle_packet(bot, header.type, payload, payload_len);
        }

        if (rx->have == sizeof(rx->buf)) {
            fprintf(stderr, "Client: rx buffer overflow / protocol desync\n");
            return -1;
        }
        break;
    }
    return 0;
}

int bot_handle_write(bot_t* bot) {
    return user_handle_write(&bot->user);
}