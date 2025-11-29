#ifndef SERVER_H
#define SERVER_H

#include "SupportStructure/fd_map.h"
#include "SupportStructure/int_map.h"
#include "Session/session.h"
#include "Server/frame.h"

#define MAX_EVENTS 64

typedef struct {
    int listen_fd;
    int epoll_fd;
    volatile int running;

    fd_map_t* response;             // client_fd -> response

    int_map_t* registered_clients; // client_id -> client_fd
    fd_map_t* registered_sessions; // session_id -> session_bus
    uint32_t next_player_id;
    uint32_t next_session_id;

} server_t;

typedef enum {P_SERVER, P_CLIENT, P_BUS} pointer_type_t;

typedef struct {
    pointer_type_t type;
    int fd;
    void* ptr;
} response_t;

int create_listen_socket(int port);
int make_nonblocking(int fd);
void tune_socket(int fd);
int create_epoll();

int add_epoll_event(int epoll_fd, int fd, uint32_t events, response_t* response);
int mod_epoll_event(int epoll_fd, int fd, uint32_t events, response_t* response);
int del_epoll_event(int epoll_fd, int fd);

int init_server(server_t* server, int port);
void cleanup_server(server_t* server);
void cleanup_client(const server_t* server, frame_t* client);

void* server_main(void* arg);

response_t* create_response(pointer_type_t type, int fd, void* ptr);
void cleanup_response(response_t* response);

void handle_accept(const server_t* server);
void handle_read(server_t* server, frame_t* frame);
void handle_write(const server_t* server, frame_t* frame);

int handle_packet_main(server_t* server, frame_t* frame);

void handle_bus_message(const server_t* server, const response_t* response);
int enqueue_message(const server_t* server, frame_t* frame, const uint8_t* buf, uint32_t length);


// Session manager
int create_new_session(server_t* server, uint8_t number_of_players);
int unregister_session(const server_t* server, session_t* session);
int validator_check_session(const server_t* server, const frame_t* frame);
uint32_t get_new_session_id(server_t* server);

// User manager
int register_client(server_t* server,const uint32_t client_id, int client_fd);
int validator_check_user(const server_t* server, const frame_t* frame);
uint32_t get_new_client_id(server_t* server);

#endif //SERVER_H
