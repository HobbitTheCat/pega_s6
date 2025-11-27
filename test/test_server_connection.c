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

int send_packet(header_t* response, const int socket, const header_t header) {
    const ssize_t sent = send(socket, &header, sizeof(header), 0);
    if (sent != sizeof(header)) {
        perror("send");
        close(socket);
        return -1;
    }
    header_t response_packet;
    const ssize_t received = recv(socket, &response_packet, sizeof(header_t), 0);
    if (received == 0) {printf("Connection closed by server\n"); close(socket); return -1;}
    if (received < 0) {perror("recv"); close(socket); return -1;}

    protocol_header_decode(&response_packet, response);
    return 0;
}

void* client(void* arg) {
    const char* host = "127.0.0.1";
    int port = 17001;

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {perror("socket"); return NULL;}

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };

    if (inet_pton(AF_INET, host, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock); return NULL;
    }
    printf("Connecting to %s:%d\n", host, port);

    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        close(sock); return NULL;
    }
    printf("Connected to %s:%d\n", host, port);

    header_t first_header;
    protocol_header_encode(&first_header, PKT_HELLO, 0, 0, sizeof(header_t));
    header_t* response = malloc(sizeof(header_t));
    send_packet(response, sock, first_header);
    print_header(response);


    header_t second_header;
    protocol_header_encode(&second_header, PKT_REGISTER, 0, 0, sizeof(header_t));
    send_packet(response, sock, second_header);
    printf("Second packet\n");
    print_header(response);

    const int user_id = response->user_id;
    header_t third_header;
    protocol_header_encode(&third_header, PKT_SESSION_CREATE, user_id, 0, sizeof(header_t));
    send_packet(response, sock, third_header);
    printf("Third packet\n");
    print_header(response);

    const int session_id = response->session_id;
    header_t fourth_header;
    protocol_header_encode(&fourth_header, PKT_START_SESSION, user_id, session_id, sizeof(header_t));
    send_packet(response, sock, fourth_header);
    printf("Forth packet\n");
    print_header(response);

    free(response);
    close(sock);
    return NULL;
}

int main() {
    server_t* server = malloc(sizeof(*server));
    if (!server) return -1;

    if (init_server(server, 17001) == -1) {free(server); perror("init server"); return -1;}

    pthread_t server_thread, client_thread;
    pthread_create(&server_thread, NULL, server_main, server);
    pthread_create(&client_thread, NULL, client, NULL);

    sleep(5);
    server->running = 0;
    cleanup_server(server);

    pthread_join(server_thread, NULL);
    pthread_join(client_thread, NULL);

    free(server);
    return 0;
}