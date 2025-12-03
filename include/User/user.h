#ifndef USER_H
#define USER_H

#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>
#include <stdatomic.h>

#include "Protocol/proto_io.h"

typedef struct {
    int sockfd;
    struct sockaddr_in server_addr;

    struct pollfd fds[2];
    nfds_t nfds;

    rx_state_t rx;
    tx_state_t tx;

    int want_pollout;
    _Atomic int is_running;
} user_t;

int user_init(user_t* user);
void user_cleanup(user_t* user);

int user_connect(user_t* user, const char* host, uint16_t port);

void user_main_loop(user_t* user);

int user_handle_read(user_t* user);
void user_handle_stdin(user_t* user);
int user_handle_write(user_t* user);

int user_enqueue_message(user_t* user, const uint8_t* buffer, uint32_t total_length);
int user_send_packet(user_t* user, uint8_t type, const void* payload, uint16_t payload_length);


int user_handle_packet(user_t* user, uint8_t type, const uint8_t* payload, uint16_t payload_length);

// Packets
int user_send_simple(user_t* user, uint8_t type);

#endif //USER_H
