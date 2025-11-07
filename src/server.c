#include "server.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENTS 64

// ---------- response_t helpers ----------
response_t* make_response(const pointer_type_t type,const int fd, void* ptr) {
    response_t* response = calloc(1, sizeof(*response));
    if (!response) return NULL;
    response->type = type;
    response->fd = fd;
    response->ptr = ptr;
    return response;
}

void free_response(response_t* response) {
    free(response);
}


int create_listen_socket(const int port) {
    const int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("server: socket");
        return -1;
    }

    const int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("server: setsockopt");
        return -1;
    }

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(fd, (struct sockaddr* )&address, sizeof(address)) == -1) {
        close(fd);
        perror("server: bind");
        return -1;
    }

    if (listen(fd, SOMAXCONN) == -1) {
        close(fd);
        perror("server: listen");
        return -1;
    }

    if (make_nonblocking(fd) == -1) return -1;
    return fd;
}

int make_nonblocking(const int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("server: fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("server: fcntl F_SETFL");
        return -1;
    }

    return 0;
}

int create_epoll(void) {
    const int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("server: epoll_create");
        return -1;
    }
    return epoll_fd;
}

// ---------- epoll wrappers (heap ptr goes into data.ptr) ----------
int add_epoll_event(const int epoll_fd, const int fd, const uint32_t events, response_t* response) {
    struct epoll_event ev = {
        .events = events,
        .data.ptr = response,
    };
    const int rc = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (rc == -1) perror("server: epoll_ctl ADD");
    return rc;
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

// ---------- server lifecycle ----------
int init_server(server_t* server, const int port) {
    server->listen_fd = create_listen_socket(port);
    if (server->listen_fd == -1) return -1;

    server->epoll_fd = create_epoll();
    if (server->epoll_fd == -1) {
        close(server->listen_fd);
        return -1;
    }

    server->registry_map = fd_map_create(256);
    server->running = 1;

    response_t* response = make_response(P_SERVER, server->listen_fd, server);
    if (!response) {
        perror("server: make_response listen_fd");
        cleanup_server(server);
        return -1;
    }

    if (add_epoll_event(server->epoll_fd, server->listen_fd, EPOLLIN | EPOLLET, response) == -1) {
        perror("server: initial_add_epoll_event");
        free_response(response);
        cleanup_server(server);
        return -1;
    }
    if (server->listen_fd >= 0 && server->listen_fd < MAX_FILE_DESCRIPTOR)
        fd_map_set(server->registry_map, server->listen_fd, response);

    return 0;
}

void cleanup_server(server_t* server) {
    if (!server) return;
    server->running = 0;

    for (int fd = 0; fd < server->registry_map->capacity; fd ++) {
        response_t* response = fd_map_get(server->registry_map, fd);
        if (!response) continue;

        if (response->type == P_CLIENT) {
            client_t* client = (client_t*)response->ptr;
            destroy_client(server, client);
        }
        if (response->type == P_EVENT || response->type == P_SERVER || response->type == P_EVENT) {
            free_response(response);
        }
        fd_map_remove(server->registry_map, fd);
    }
    fd_map_destroy(server->registry_map);
    del_epoll_event(server->epoll_fd, server->listen_fd);
    close(server->listen_fd);
    close(server->epoll_fd);
}

void* io_thread_main(void* arg) {
    server_t* server = arg;
    struct epoll_event events[MAX_EVENTS];

    while (server->running) {
        int const n = epoll_wait(server->epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            if (errno == EINTR) continue;
            perror("server: epoll_wait");
            cleanup_server(server);
            break;
        };
        for (int i = 0; i < n; i++) {
            const uint32_t event = events[i].events;
            const response_t* response = (response_t*)events[i].data.ptr;
            if (!response) continue;

            if (event & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                if (response->type == P_CLIENT) {
                    destroy_client(server, (client_t*)response->ptr);
                }
                continue;
            }

            if (response->type == P_SERVER) {
                handle_accept(server);
            } else if (response->type == P_CLIENT) {
                client_t* client = (client_t*)response->ptr;
                if (event & EPOLLIN) handle_read(server, client);
                if (event & EPOLLOUT) handle_write(server, client);
            } else if (response->type == P_EVENT) {
                // тут обработать событие связанное с буферами
            } else if (response->type == P_BUS) {
                // тут обработать событие связанное с шиной событий из SessionManager
            }
        }
    }

    return NULL;
}

void handle_accept(const server_t* server) {
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);

        const int client_fd = accept(server->listen_fd, (struct sockaddr*) &client_address, &client_address_len);
        if (client_fd == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            perror("server: accept");
            break;
        }
        if (make_nonblocking(client_fd) == -1) continue;

        client_t* client = calloc(1, sizeof(client_t));
        if (!client) {close(client_fd); continue;}
        client->fd = client_fd;
        reset_to_header(client);
        client->have = 0;

        response_t* response = make_response(P_CLIENT, client_fd, client);
        if (!response) {
            close(client_fd);
            free(client);
            continue;
        }

        if (add_epoll_event(server->epoll_fd, client_fd, EPOLLIN | EPOLLET, response) == -1) {
            free_response(response);
            close(client_fd);
            free(client);
            continue;
        }
        if (client_fd >= 0 && client_fd < MAX_FILE_DESCRIPTOR)
            fd_map_set(server->registry_map, client->fd, response);

        printf("New connection: fd=%d\n", client_fd);
    }
}

void set_payload_from_header(client_t* client, const header_t* header) {
    client->need = header->length;
    client->user_id = header->user_id;
    client->session_id = header->session_id;
    client->state = R_PAYLOAD;
}

void reset_to_header(client_t* client) {
    client->state = R_HEADER;
    client->need = sizeof(header_t);
}

void handle_read(server_t* server, client_t* client) {
    for (;;) {
        // Читать пока есть место и пока ядро отдает данные
        while (client->have < sizeof(client->buf)) {
            const ssize_t n = recv(client->fd, client->buf + client->have, sizeof(client->buf) - client->have, 0);
            if (n > 0) {  // что-то было прочитано
                client->have += (uint32_t)n;
                continue; // прочитаем еще, если есть
            }
            if (n == 0) { // клиент прервал соединение, можно закрывать
                destroy_client(server, client);
                return;
            }
            // если ошибка
            if (errno == EINTR) continue; // можно повторить получение
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // были прочитаны все данные, переходим к обработке
            perror("server: recv"); // возникла другая ошибка
            destroy_client(server, client);
            return;
        }

        // Обработка того, что есть в буфере
        for (;;) {
            if (client->state == R_HEADER) {
                if (client->have < sizeof(header_t)) break; // Не достаточно для заголовка ждем еще
                header_t header;
                const int r = try_peek_header(client->buf, client->have, &header);
                if (r == -1) break; // байтов мало ждем еще
                if (r == -2) { // ошибка декодирования или ошибка соединения
                    printf("Возникла та самая ошибка с неправильным пакетом, проверь порядок байтов дурак");
                    destroy_client(server, client);
                    return;
                }
                set_payload_from_header(client, &header); // переводим в режим чтения нагрузки
                if (client->have < client->need) break;
            }

            if (client->state == R_PAYLOAD) {
                if (client->have < client->need) break;

                // TODO тут нужен код для работы с готовым буфером

                const uint32_t remain = client->have - client->need;
                if (remain) memmove(client->buf, client->buf + client->need, remain); //вырезаем кадр из начала буфера
                client->have = remain;
                reset_to_header(client); // переводим в режим получения заголовка
                continue;
            }

            break;
        }
        // Если дошли до сюда, то либо нет новых данных, либо в буфере не достаточно данных - выходим
        break;
    }
    // Если буфер забит, но мы все еще ждем больше - проблема протокола, сбрасываем соединение
    if (client->have == sizeof(client->buf) && client->have < client->need) {
        destroy_client(server, client);
    }
}


void handle_write(const server_t* s,const  client_t* c) {
    // TODO: отправка из очереди (ring buffer); при опустошении убрать EPOLLOUT
    (void)s; (void)c;
}


void destroy_client(const server_t* server, client_t* client) {
    if (!client) return;
    if (server->epoll_fd >= 0) del_epoll_event(server->epoll_fd, client->fd);
    close(client->fd);
    if (client->fd >= 0 && client->fd < MAX_FILE_DESCRIPTOR) {
        response_t* response = fd_map_get(server->registry_map, client->fd);
        if (response) {
            fd_map_remove(server->registry_map, client->fd);
            free_response(response);
        }
    }
    free(client);
}
