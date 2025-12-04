#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// #include "Protocol/protocol.h"
// #include "Server/server.h"
#include "../include/Protocol/protocol.h"
#include "../include/Server/server.h"
#include "../include/User/user.h"

int main() {
    server_t* server = malloc(sizeof(*server));
    if (!server) return -1;

    if (init_server(server, 17001) == -1) {free(server); perror("init server"); return -1;}

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_main, server);

    user_t user;
    user_init(&user);
    user_run(&user, "127.0.0.1", 17001);

    server->running = 0;
    cleanup_server(server);

    pthread_join(server_thread, NULL);

    free(server);
    return 0;
}
