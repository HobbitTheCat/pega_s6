#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/User/bot.h"
#include "../include/User/user.h"

int bot_step(bot_t* bot) {
    user_t* user = &bot->user;
    if (!user->is_running || user->sockfd < 0) return -1;

    const int ret = poll(user->fds, user->nfds, 100);
    if (ret < 0) return -1;

    if (ret > 0) {
        if (user->fds[1].revents & POLLIN) if (bot_handle_read(bot) < 0) return -1;
        if (user->fds[1].revents & POLLOUT) if (bot_handle_write(bot) < 0) return -1;
    }
    return 0;
}

int main() {
    const char* host = "127.0.0.1";
    const int port = 17001;

    bot_t* bot = malloc(sizeof(bot_t));
    if (!bot) return -1;
    bot_init(bot);
    if (bot_connect(bot, host, port) < 0) {perror("Master connect failed"); bot_cleanup(bot); free(bot); return -1;}
    while (bot->session_id == 0 && bot->user.is_running)
        if (bot_step(bot) < 0) break;
    if (bot->session_id == 0){ fprintf(stderr, "Failed to create session or get ID\n"); bot_cleanup(bot); free(bot);return -1;}

    const int session_id = bot->session_id;
    printf("Session Created! ID: %d. Spawning bots...\n", session_id);

    const int bots_to_create = 3;
    for (int i = 0; i < bots_to_create; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            bot_t* sub_bot = malloc(sizeof(bot_t));
            bot_init(sub_bot);
            printf("[Child %d] Connecting to session %d\n", i, session_id);
            bot_start(sub_bot, host, port, session_id);
            free(sub_bot);
            exit(0);
        }
    }
    printf("Master bot continuing execution...\n");
    bot_run(bot);
    free(bot);
    return 0;
}