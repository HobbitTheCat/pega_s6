#include "Server/server.h"

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
    if (server->epoll_fd == -1){ close(server->listen_fd); return -1;}

    server->response = fd_map_create(256);
    server->registered_sessions = fd_map_create(32);
    server->registered_clients = int_map_create(256);
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

    for (int fd = 0; fd < server->response->capacity; fd++) {
        response_t* response = fd_map_get(server->response, fd);
        if (!response) continue;

        if (response->type == P_CLIENT) {
            cleanup_client(server, response->ptr);
        } else {
            fd_map_remove(server->response, fd);
            free(response);
        }
    }
    fd_map_destroy(server->response);
    fd_map_destroy(server->registered_sessions);
    int_map_destroy(server->registered_clients);

    del_epoll_event(server->epoll_fd, server->listen_fd);
    close(server->listen_fd);
    close(server->epoll_fd);
}


void cleanup_client(const server_t* server, frame_t* client) {
    if (!client) return;
    del_epoll_event(server->epoll_fd, client->fd);
    shutdown(client->fd, SHUT_RDWR);
    close(client->fd);
    response_t* response = fd_map_get(server->response, client->fd);
    if (response) {
        fd_map_remove(server->response, client->fd);
        free(response);
    }
    free_tx_queue(&client->tx);
    free(client);
}

void* server_main(void* arg) {
    server_t* server = arg;
    struct epoll_event events[MAX_EVENTS];

    while (server->running) {
        int const n = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1){ if (errno == EINTR) continue; perror("server: epoll_wait"); cleanup_server(server); break;}

        for (int i = 0; i < n; i++) {
            const uint32_t event = events[i].events;
            const response_t* response = (response_t*) events[i].data.ptr;
            if (!response) continue;

            if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                if (response->type == P_CLIENT) {
                    cleanup_client(server, response->ptr);
                }
                continue;
            }

            if (response->type == P_SERVER) {
                handle_accept(server);
            } else if (response->type == P_CLIENT) {
                frame_t* frame = response->ptr;
                if (event & EPOLLIN) handle_read(server, frame);
                if (event & EPOLLOUT) handle_write(server, frame);
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

        frame_t* client = calloc(1, sizeof(*client));
        if (!client) {close(client_fd); continue;}
        init_client(client, client_fd);

        response_t* response = create_response(P_CLIENT, client->fd, client);
        if (!response) {close(client_fd); free(client); continue;}

        if (add_epoll_event(server->epoll_fd, client_fd, EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET, response) == -1) {
            free(response); close(client_fd); free(client); continue;
        }

        fd_map_set(server->response, client_fd, response);
        printf("server: accepted connection from %s\n", inet_ntoa(client_address.sin_addr));
    }
}

// void handle_read(server_t* server, frame_t* frame) {
//     uint8_t* buf = frame->rx.buf;
//     const uint32_t capacity = sizeof (frame->rx.buf);
//     for (;;) {
//         while (frame->rx.write_pos != capacity) {
//             const ssize_t n = recv(frame->fd, buf + frame->rx.write_pos, capacity - frame->rx.write_pos, 0);
//             if (n > 0) {frame->rx.write_pos += n; continue;}
//             if (n == 0) {cleanup_client(server, frame); return;}
//             if (errno == EINTR) continue;
//             if (errno == EAGAIN || errno == EWOULDBLOCK) break;
//             perror("server: recv"); cleanup_client(server, frame); return;
//         }
//
//         for (;;) {
//             const uint32_t available = frame->rx.write_pos - frame->rx.read_pos;
//             if (frame->rx.state == R_HEADER) {
//                 header_t header;
//                 const int r = try_peek_header(buf + frame->rx.read_pos, available, &header);
//                 if (r == -1) break;
//                 if (r == -2) {cleanup_client(server, frame); return;}
//                 frame->rx.need = header.length;
//                 frame->user_id = header.user_id;
//                 frame->session_id = header.session_id;
//                 frame->packet_type = header.type;
//                 frame->rx.state = R_PAYLOAD;
//                 if (available < frame->rx.need) break;
//             }
//
//             if (frame->rx.state == R_PAYLOAD) {
//                 if (available < frame->rx.need) break;
//
//
//                 frame->rx.read_pos += frame->rx.need;
//                 frame->rx.state = R_HEADER;
//                 continue;
//             }
//             break;
//         }
//
//         if (frame->rx.state == frame->rx.write_pos) {
//             frame->rx.read_pos = frame->rx.write_pos = 0;
//         } else if (frame->rx.write_pos == capacity && frame->rx.read_pos > 0) {
//             const uint32_t remain = frame->rx.write_pos - frame->rx.read_pos;
//             memmove(buf, buf + frame->rx.read_pos, remain);
//             frame->rx.write_pos = remain;
//             frame->rx.read_pos = 0;
//         }
//         if (errno == EAGAIN || errno == EWOULDBLOCK) break;
//     }
//
//     if (frame->rx.write_pos == capacity && frame->rx.state == R_PAYLOAD) {
//         cleanup_client(server, frame);
//     }
// }

void handle_read(server_t* server, frame_t* frame) {
    for (;;) {
        while (frame->rx.have < sizeof(frame->rx.buf)) {
            const ssize_t n = recv(frame->fd, frame->rx.buf + frame->rx.have, sizeof(frame->rx.buf) - frame->rx.have, 0);
            if (n > 0) {frame->rx.have += (uint32_t)n; continue;}
            if (n == 0) { cleanup_client(server, frame); return;} //TODO не забыть очистить registered_clients
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("server: recv");
            cleanup_client(server, frame); //TODO не забыть очистить registered_clients
            return;;
        }

        for (;;) {
            if (frame->rx.state == R_HEADER) {
                if (frame->rx.have < sizeof(header_t)) break;
                header_t header;
                const int r = try_peek_header(frame->rx.buf, frame->rx.have, &header);
                if (r == -1) break;
                if (r == -2) {
                    printf("server: received bad header\n");
                    cleanup_client(server, frame);  //TODO не забыть очистить registered_clients
                    return;
                }
                set_payload_from_header(frame, &header);
                if (frame->rx.have < frame->rx.need) break;
            }

            if (frame->rx.state == R_PAYLOAD) {
                if (frame->rx.have < frame->rx.need) break;

                printf("Received payload.\n");
                handle_packet_main(server, frame);
                // TODO вот тут то все и обрабатывается

                const uint32_t remain = frame->rx.have - frame->rx.need;
                if (remain) memmove(frame->rx.buf, frame->rx.buf + frame->rx.need, remain);
                frame->rx.have = remain;
                reset_to_header(frame);
                continue;
            }
            break;
        }
        break;
    }
    if (frame->rx.have == sizeof(frame->rx.buf) && frame->rx.have < frame->rx.need) {
        cleanup_client(server, frame);
    }
}

void handle_write(const server_t* server, frame_t* frame) {
    while (frame->tx.head) {
        out_chunk_t* chunk = frame->tx.head;
        while (chunk->off < chunk->len) {
            const ssize_t n = send(frame->fd, chunk->data + chunk->off, chunk->len - chunk->off, MSG_NOSIGNAL);
            if (n > 0) {
                chunk -> off += (uint32_t)n;
                frame->tx.queued -= (size_t)n;
                continue;
            }
            if (n < 0 && errno == EINTR) continue;
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
            cleanup_client(server, frame);
            return;
        }
        frame->tx.head = chunk->next;
        if (!frame->tx.head) frame->tx.tail = NULL;
        free(chunk->data);
        free(chunk);
    }
    if (frame->tx.epollout == 1) {
        mod_epoll_event(server->epoll_fd, frame->fd,
            EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET,
            fd_map_get(server->response, frame->fd));
        frame->tx.epollout = 0;
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