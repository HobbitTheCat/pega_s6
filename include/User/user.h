#ifndef USER_H
#define USER_H

#include <netinet/in.h>
#include <poll.h>
#include <stdint.h>

#include "Protocol/proto_io.h"

typedef struct {
    uint32_t user_id;
    uint32_t session_id;
    packet_type_t packet_type;

    int sockfd;
    struct sockaddr_in server_addr;

    struct pollfd fds[2];
    nfds_t nfds;

    rx_state_t rx;
    tx_state_t tx;

    int want_pollout;
    int is_running;
} user_t;

int user_init(user_t* user);
void user_cleanup(user_t* user);

int user_connect(user_t* user, const char* host, uint16_t port);

void user_main_loop(user_t* user);

int user_handle_read(user_t* user);
void user_handle_stdin(user_t* user);
int user_handle_write(user_t* user);

int user_enqueue_message(user_t* user, const uint8_t* buf, uint32_t length);
int user_handle_packet(user_t* user);

void user_set_payload_from_header(user_t* user, const header_t* header);

#endif //USER_H
