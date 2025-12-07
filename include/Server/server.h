#ifndef SERVER_H
#define SERVER_H

#include "session_bus.h"
#include "SupportStructure/fd_map.h"
#include "Server/server_conn.h"

#define MAX_EVENTS 64

typedef struct {
    int listen_fd;
    int epoll_fd;
    volatile int running;

    fd_map_t* response;            // client_fd -> response

    fd_map_t* registered_players;  // player_id -> server_player
    fd_map_t* registered_sessions; // session_id -> session
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
void cleanup_connection(server_t* server, server_conn_t* conn);

void* server_main(void* arg);

response_t* create_response(pointer_type_t type, int fd, void* ptr);
void cleanup_response(response_t* response);

void handle_accept(const server_t* server);
void handle_read(server_t* server, server_conn_t* conn);
void handle_write(server_t* server, server_conn_t* conn);

int enqueue_message(server_t* server, server_conn_t* conn, uint8_t* buffer, uint32_t total_length) ;
int send_packet(server_t* server, server_conn_t* conn, uint8_t type, const void* payload, uint16_t payload_length);

void handle_packet_main(server_t* server, server_conn_t* conn, uint8_t packet_type, const uint8_t* payload, uint16_t payload_length);

// Session manager
int send_message_to_session(const server_t* server, uint32_t session_id, uint32_t user_id, uint8_t packet_type, const uint8_t* payload, uint16_t payload_length);
int send_system_message_to_session(const server_t* server, uint32_t session_id, uint32_t user_id, system_message_type_t type);
void handle_bus_message(server_t* server, const response_t* response);
int create_new_session(server_t* server, uint8_t number_of_players, uint8_t is_visible);
int unregister_session(const server_t* server, uint32_t session_id);
int get_available_sessions(const server_t* server, uint32_t* session_list);
uint32_t get_new_session_id(server_t* server);

// Client manager
int generate_reconnection_token(char* buf, size_t len);
int register_client(server_t* server, server_conn_t* conn);
int disconnect_client(server_t* server, server_conn_t* conn);
int reconnect_client(server_t* server, server_conn_t* conn, const uint32_t client_id, const char* reconnection_token);
int unregister_client(server_t* server, uint32_t player_id);
uint32_t get_new_client_id(server_t* server);

// Packet
int send_simple_packet(server_t* server, server_conn_t* conn, uint8_t type);
int send_error_packet(server_t* server, server_conn_t* conn, uint8_t error_code, const char* error_message);
int send_sync_state(server_t* server, server_conn_t* conn);
int send_session_list(server_t* server, server_conn_t* conn);

int handle_reconnect_packet(server_t* server, server_conn_t* conn, const uint8_t* payload, uint16_t payload_length);
int handle_session_create_packet(server_t* server, server_conn_t* conn, const uint8_t* payload, uint16_t payload_length);
uint32_t handle_session_join_packet(server_t* server, server_conn_t* conn, const uint8_t* payload, uint16_t payload_length);
#endif //SERVER_H
