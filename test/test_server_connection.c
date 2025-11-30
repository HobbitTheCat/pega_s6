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

// void* client(void* arg) {
//     const char* host = "127.0.0.1";
//     int port = 17001;
//
//     user_t user;
//     if (user_connect(&user, host, port) < 0) {
//         perror("user_connect");
//         return NULL;
//     }
//
//     header_t first_header;
//     protocol_header_encode(&first_header, PKT_HELLO, 0, 0, sizeof(header_t));
//     header_t* response = malloc(sizeof(header_t));
//     send_packet(response, sock, first_header);
//     print_header(response);
//
//
//     header_t second_header;
//     protocol_header_encode(&second_header, PKT_REGISTER, 0, 0, sizeof(header_t));
//     send_packet(response, sock, second_header);
//     printf("Second packet\n");
//     print_header(response);
//
//     const int user_id = response->user_id;
//     header_t third_header;
//     protocol_header_encode(&third_header, PKT_SESSION_CREATE, user_id, 0, sizeof(header_t));
//     send_packet(response, sock, third_header);
//     printf("Third packet\n");
//     print_header(response);
//
//     const int session_id = response->session_id;
//     header_t fourth_header;
//     protocol_header_encode(&fourth_header, PKT_START_SESSION, user_id, session_id, sizeof(header_t));
//     send_packet(response, sock, fourth_header);
//     printf("Forth packet\n");
//     print_header(response);
//
//     free(response);
//     close(sock);
//     return NULL;
// }
int main() {
    server_t* server = malloc(sizeof(*server));
    if (!server) return -1;

    if (init_server(server, 17001) == -1) {free(server); perror("init server"); return -1;}

    pthread_t server_thread;
    pthread_create(&server_thread, NULL, server_main, server);

    user_t user;
    user_init(&user);
    if (user_connect(&user, "127.0.0.1", 17001) == -1) {
        perror("user_connect");
        server->running = 0;
        pthread_join(server_thread, NULL);
        cleanup_server(server);
        return -1;
    }

    user_main_loop(&user);

    server->running = 0;
    cleanup_server(server);

    pthread_join(server_thread, NULL);

    free(server);
    return 0;
}
