#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

#define SP_MAX_FRAME 4096
#define MAX_FILE_DESCRIPTOR 65536


typedef enum {R_HEADER, R_PAYLOAD} recv_state_t;

typedef struct {
    int fd;
    recv_state_t state;
    uint8_t buf[SP_MAX_FRAME];
    uint32_t have;
    uint32_t need;

    uint32_t user_id;
    uint32_t session_id;
} client_t;

typedef struct {
    int listen_fd;
    int epoll_fd;
    volatile int running;

    client_t* registry[MAX_FILE_DESCRIPTOR];
} server_t;

typedef enum {SERVER, CLIENT, EVENT} pointer_t;
typedef struct {
    pointer_t type;
    void* ptr;
} response_t;


int create_listen_socket(int port);
int make_nonblocking(int fd);
int create_epoll(void);

int add_epoll_listen(int epoll_fd, int listen_fd, uint32_t events);
int add_epoll_client(int epoll_fd, int fd, uint32_t events, client_t* client);
int mod_epoll_event(int epoll_fd, int fd, uint32_t events, client_t* client);
int del_epoll_event(int epoll_fd, int fd);

int init_server(server_t* server, int port);
void cleanup_server(server_t* server);

void* io_thread_main(void* arg);

void handle_accept(server_t* server);
void handle_read(server_t* server, client_t* client);
void handle_write(server_t* server, client_t* client);

void destroy_client(server_t* server, client_t* client);

#endif //SERVER_H
