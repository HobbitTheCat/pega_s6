#include <stdio.h>
#include <stdlib.h>

#include "../include/User/bot.h"
#include "../include/Protocol/protocol.h"

int main () {
    bot_t* bot = malloc(sizeof(bot_t));
    if (!bot) return -1;

    bot_init(bot);
    bot_start(bot, "127.0.0.1", 17001, 1, 2);

    free(bot);
    return 0;
}