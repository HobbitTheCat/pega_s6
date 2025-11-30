#include "User/user.h"

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int user_init(user_t* user) {
    if (!user) return -1;
    memset(user, 0, sizeof(*user));
    user->sockfd = -1;

    rx_init(&user->rx);
    tx_init(&user->tx);

    user->fds[0].fd = STDIN_FILENO;
    user->fds[0].events = POLLIN;
    user->nfds = 1;

    user->is_running = 1;
    return 0;
}

int user_connect(user_t* user, const char* host, const uint16_t port) {
    if (!user || !host){errno = EINVAL; return -1;}

    user->sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); //TODO проверить что NONBLOCKING
    if (user->sockfd < 0){perror("Client: Socket"); return -1;}

    memset(&user->server_addr, 0, sizeof(user->server_addr));
    user->server_addr.sin_family = AF_INET;
    user->server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &user->server_addr.sin_addr) <= 0) {
        perror("Client: inet_pton");
        close(user->sockfd); user->sockfd = -1; return -1;
    }

    if (connect(user->sockfd, (struct sockaddr*)&user->server_addr,
            sizeof(user->server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("Client: connect");
            close(user->sockfd); user->sockfd = -1; return -1;
        }
    }

    user->fds[0].fd = STDIN_FILENO;
    user->fds[0].events = POLLIN;
    user->fds[1].fd = user->sockfd;
    user->fds[1].events = POLLIN;
    user->want_pollout = 0;
    user->nfds = 2;

    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;
    const int packet_size = create_simple_packet(buffer, PKT_HELLO, 0, 0);
    user_enqueue_message(user, buffer, packet_size);

    free(buffer);
    return 0;
}

void user_cleanup(user_t* user) {
    if (!user) return;
    if (user->sockfd >= 0) {
        close(user->sockfd);
        user->sockfd = -1;
    }
    tx_free_queue(&user->tx);
}

void user_main_loop(user_t* user) {
    if (!user || user->sockfd < 0) return;

    while (user->is_running) {
        const int ret = poll(user->fds, user->nfds, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("Client: poll"); break;
        }

        if (user->fds[0].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
            user_handle_stdin(user); //TODO переписать для стандартного ввода
        }

        if (user->fds[1].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL | POLLOUT)) {
            if (user->fds[1].revents & POLLIN) {
                if (user_handle_read(user) < 0) break;
            }
            if (user->fds[1].revents & POLLOUT) {
                if (user_handle_write(user) < 0) break;
                if (!user->tx.head) { user->fds[1].events &= ~POLLIN; user->want_pollout = 0;}
            }
            if (user->fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                perror("Client: unknown error"); break;
            }
        }
    }
}

int user_handle_read(user_t* user) {
    for (;;) {
        while (user->rx.have < sizeof(user->rx.buf)) {
            const ssize_t n = recv(user->sockfd, user->rx.buf + user->rx.have, sizeof(user->rx.buf) - user->rx.have, 0);
            if (n > 0) {user->rx.have += (uint32_t)n; continue;}
            if (n == 0) {printf("Client: Server closed connection\n"); return -1;}
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("Client: recv"); return -1;
        }

        for (;;) {
            if (user->rx.state == R_HEADER) {
                if (user->rx.have < sizeof(header_t)) break;
                header_t header;
                const int r = try_peek_header(user->rx.buf, user->rx.have, &header);
                if (r == -1) break;
                if (r == -2) {fprintf(stderr, "Client: bad header from server\n"); return -1;}

                user_set_payload_from_header(user, &header);
                if (user->rx.have < user->rx.need) break;
            }

            if (user->rx.state == R_PAYLOAD) {
                if (user->rx.have < user->rx.need) break;

                user_handle_packet(user);
                // TODO вот тут то все и обрабатывается

                const uint32_t remain = user->rx.have - user->rx.need;
                if (remain) memmove(user->rx.buf, user->rx.buf + user->rx.need, remain);
                rx_reset_to_header(&user->rx, remain);
                continue;
            }
            break;
        }
        break;
    }
    if (user->rx.have == sizeof(user->rx.buf) && user->rx.have < user->rx.need) {
        fprintf(stderr, "Client: frame too large, closing\n"); return -1;
    }
    return 0;
}

int user_handle_write(user_t* user) {
    while (user->tx.head) {
        out_chunk_t* chunk = user->tx.head;
        while (chunk->off < chunk->len) {
            const ssize_t n = send(user->sockfd, chunk->data + chunk->off, chunk->len - chunk->off, MSG_NOSIGNAL);
            if (n > 0) {
                chunk->off += (uint32_t)n;
                user->tx.queued -= (size_t)n;
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && errno == EAGAIN || errno == EWOULDBLOCK) return 0;
            perror("Client: send"); return -1;
        }
        user->tx.head = chunk->next;
        if (!user->tx.head) user->tx.tail = NULL;
        free(chunk->data);
        free(chunk);
    }
    if (user->want_pollout) {
        user->fds[1].events &= ~POLLOUT;
        user->want_pollout = 0;
    }
    return 0;
}

int user_enqueue_message(user_t* user, const uint8_t* buf, const uint32_t length) {
    if (!user || user->sockfd < 0 || !buf) {errno = EINVAL; return -1;}
    if (user->tx.queued + length > user->tx.high_watermark) {errno = EAGAIN; return -1;}

    out_chunk_t* chunk = malloc(sizeof(*chunk));
    if (!chunk) return -1;
    chunk->data = malloc(length);
    if (!chunk->data) {free(chunk); return -1;}
    memcpy(chunk->data, buf, length);
    chunk->len = length;
    chunk->off = 0;
    chunk->next = NULL;

    if (!user->tx.tail) user->tx.head = user->tx.tail = chunk;
    else {user->tx.tail->next = chunk; user->tx.tail = chunk;}

    user->tx.queued += length;
    if (user_handle_write(user) < 0) return -1;

    if (user->tx.head && user->want_pollout == 0) {
        user->fds[1].events |= POLLOUT;
        user->want_pollout = 1;
    }
    return 0;
}

void user_set_payload_from_header(user_t* user, const header_t* header) {
    rx_set_payload(&user->rx, header->length);
    user->user_id = header->user_id;
    user->session_id = header->session_id;
    user->packet_type = header->type;
}