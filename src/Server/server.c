#include "Server/server.h"
#include "Server/server_conn.h"
#include "Session/session.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>

int create_listen_socket(const int port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {perror("server_init: socket"); return -1;}

    const int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        close(fd); perror("server_init: setsockopt");  return -1;
    }

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        close(fd); perror("server_init: bind");  return -1;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        close(fd); perror("server_init: listen");  return -1;
    }

    if (make_nonblocking(fd) == -1) {
        close(fd); perror("server_init: make_nonblocking");  return -1;
    }
    return fd;
}

int make_nonblocking(const int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {perror("server: make_nonblocking fcntl F_GETFL"); return -1;}
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {perror("server: make_nonblocking fcntl F_SETF"); return -1;}
    return 0;
}

void tune_socket(const int fd) {
    const int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
}

// --------- epoll ---------

int create_epoll() {
    const int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {perror("server_init: epoll_create1"); return -1;}
    return epoll_fd;
}

int add_epoll_event(const int epoll_fd,const int fd,const uint32_t events, response_t* response) {
    struct epoll_event ev = {
        .events = events,
        .data.ptr = response
    };
    const int result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (result == -1) {perror("server: epoll_ctl ADD"); return -1;}
    return result;
}

int mod_epoll_event(const int epoll_fd, const int fd, const uint32_t events, response_t* response) {
    struct epoll_event ev = {
        .events = events,
        .data.ptr = response,
    };
    const int rc = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    if (rc == -1) perror("epoll_ctl MOD");
    return rc;
}

int del_epoll_event(const int epoll_fd, const int fd) {
    const int result = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    if (result == -1) perror("server: epoll_ctl_del");
    return result;
}

// --------- server ---------
int init_server(server_t* server, const int port) {
    server->listen_fd = create_listen_socket(port);
    if (server->listen_fd == -1) return -1;

    server->epoll_fd = create_epoll();
    if (server->epoll_fd == -1) {close(server->listen_fd); return -1;}

    server->response = fd_map_create(256);
    server->registered_sessions = fd_map_create(32);
    server->registered_players = fd_map_create(256);
    server->next_player_id = 1;
    server->next_session_id = 1;

    server->running = 1;
    response_t* response = create_response(P_SERVER, server->listen_fd, server);
    if (!response) {cleanup_server(server); return -1;}
    if (add_epoll_event(server->epoll_fd, server->listen_fd, EPOLLIN, response) == -1) {
        perror("server_init: add_epoll_event"); free(response); cleanup_server(server); return -1;
    }
    fd_map_set(server->response, server->listen_fd, response);
    return 0;
}

void cleanup_server(server_t* server) {
    if (!server) return;
    server->running = 0;

    for (int session_id = 1; session_id < server->next_session_id; session_id++) {
        server_session_t* session_info = fd_map_get(server->registered_sessions, session_id);
        if (!session_info) continue;
        unregister_session(server, session_id);
    } fd_map_destroy(server->registered_sessions);

    for (int fd = 0; fd < server->response->capacity; fd++) {
        response_t* response = fd_map_get(server->response, fd);
        if (!response) continue;
        switch (response->type) {
            case P_CLIENT: cleanup_connection(server, response->ptr); break;
            default:
                fd_map_remove(server->response, fd);
                free(response);
        }
    }
    fd_map_destroy(server->response);

    for (int player_id = 1; player_id < server->next_player_id; player_id++) {
        server_player_t* player = fd_map_get(server->registered_players, (int)player_id);
        if (!player) continue;
        free(player->reconnection_token);
        free(player);
    }
    fd_map_destroy(server->registered_players);
    del_epoll_event(server->epoll_fd, server->listen_fd);
    close(server->listen_fd);
    close(server->epoll_fd);
}

void cleanup_connection(server_t* server, server_conn_t* conn) {
    if (!conn) return;
    if (conn->player != NULL) disconnect_client(server, conn);
    del_epoll_event(server->epoll_fd, conn->fd);
    shutdown(conn->fd, SHUT_RDWR);
    close(conn->fd);
    response_t* response = fd_map_get(server->response, conn->fd);
    if (response) {
        fd_map_remove(server->response, conn->fd);
        free(response);
    }
    server_conn_free_tx(conn);
    free(conn);
}

void* server_main(void* arg) {
    server_t* server = arg;
    struct epoll_event events[MAX_EVENTS];
    // TODO если во время внутреннего цикла первым сообщением освобождается клиент а вторым приходит ему лично то происходит NULL PINTER EXCEPTION решение - видимо стэк
    while (server->running) {
        int const n = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1); // TODO убрать 20
        if (n == -1){ if (errno == EINTR) continue; perror("server: epoll_wait"); cleanup_server(server); break;}

        for (int i = 0; i < n; i++) {
            const uint32_t event = events[i].events;
            const response_t* response = (response_t*) events[i].data.ptr;
            if (!response) continue;

            if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                if (response->type == P_CLIENT) {
                    printf("Вызываем тут очистку 1\n");
                    cleanup_connection(server, response->ptr);
                }
                continue;
            }

            if (response->type == P_SERVER) {
                handle_accept(server);
            } else if (response->type == P_CLIENT) {
                server_conn_t* conn = response->ptr;
                if (event & EPOLLIN) handle_read(server, conn);
                if (event & EPOLLOUT) handle_write(server, conn);
            } else if (response->type == P_BUS) {
                handle_bus_message(server, response);
            }
        }
    }
    return NULL;
}

void handle_accept(const server_t* server) {
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);

        const int client_fd = accept(server->listen_fd, (struct sockaddr*) &client_address, &client_address_length);
        if (client_fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) break;
            perror("server: accept"); break;
        }
        if (make_nonblocking(client_fd) == -1) continue;

        server_conn_t* conn = calloc(1, sizeof(*conn));
        if (!conn) {close(client_fd); continue;}
        server_conn_init(conn, client_fd);

        response_t* response = create_response(P_CLIENT, conn->fd, conn);
        if (!response) {close(client_fd); free(conn); continue;}

        if (add_epoll_event(server->epoll_fd, client_fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET, response) == -1) {
            free(response); close(client_fd); free(conn); continue;
        }

        fd_map_set(server->response, client_fd, response);
        printf("server: accepted connection from %s\n", inet_ntoa(client_address.sin_addr));
    }
}

void handle_read(server_t* server, server_conn_t* conn) {
    rx_state_t* rx = &conn->rx;

    for (;;) {
        while (rx->have < sizeof(rx->buf)) {
            const ssize_t n = recv(conn->fd, rx->buf + rx->have, sizeof(rx->buf) - rx->have, 0);
            if (n > 0) {rx->have += n; continue;}
            if (n == 0) {printf("Вызываем тут очистку 2\n"); cleanup_connection(server, conn); return;}
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            printf("Вызываем тут очистку 3\n");
            perror("server: recv"); cleanup_connection(server, conn); return;
        }

        for (;;) {
            header_t header;
            uint8_t* payload;
            uint16_t payload_len;

            const int r = rx_try_extract_frame(rx, &header, &payload, &payload_len);
            if (r == 0) break;
            if (r == -1) {
                printf("server: received bad packet\n");
                cleanup_connection(server, conn);
                return;
            }
            handle_packet_main(server, conn, header.type, payload, payload_len);
        }
        if (rx->have < sizeof(rx->buf)) break;
        if (rx->have == sizeof(rx->buf)) {
            printf("server: rx buffer overflow / protocol desync\n");
            cleanup_connection(server, conn);
        }
        break;
    }
}

void handle_write(server_t* server, server_conn_t* conn) {
    while (conn->tx.head) {
        out_chunk_t* chunk = conn->tx.head;
        while (chunk->off < chunk->len) {
            const ssize_t n = send(conn->fd, chunk->data + chunk->off, chunk->len - chunk->off, MSG_NOSIGNAL);
            if (n > 0) {
                chunk -> off += (uint32_t)n;
                conn->tx.queued -= (size_t)n;
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            printf("Вызываем тут очистку 5\n");
            cleanup_connection(server, conn);
            return;
        }
        conn->tx.head = chunk->next;
        if (!conn->tx.head) conn->tx.tail = NULL;
        free(chunk->data);
        free(chunk);
    }
    if (conn->want_epollout) {
        mod_epoll_event(server->epoll_fd, conn->fd,
            EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET,
            fd_map_get(server->response, conn->fd));
        conn->want_epollout = 0;
    }
}

response_t* create_response(const pointer_type_t type,const int fd, void* ptr) {
    response_t* response = calloc(1, sizeof(*response));
    if (!response) return NULL;
    response->type = type;
    response->fd = fd;
    response->ptr = ptr;
    return response;
}

void cleanup_response(response_t* response) {
    free(response);
}