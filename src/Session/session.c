#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>

#include "Session/session.h"

int init_session(session_t* session, const uint32_t session_id, const uint8_t capacity) {
    session->capacity = capacity;
    session->num_players = 0;
    session->id = session_id;

    session->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (session->epoll_fd == -1) {perror("session: epoll_create");return -1;}

    session->players = calloc(capacity, sizeof(player_t));
    if (!session->players) {
        perror("session: malloc clients");
        close(session->epoll_fd);
        return -1;
    }

    session->bus = create_session_bus(256, session->id);
    if (!session->bus) {
        perror("session: create_session_bus");
        close(session->epoll_fd);
        free(session->players);
        return -1;
    }

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.ptr = &session->bus->read.queue
    };
    epoll_ctl(session->epoll_fd, EPOLL_CTL_ADD, session->bus->read.event_fd, &ev);
    session->running = 1;
    return 0;
}

void cleanup_session_fist_step(const session_t* session) {
    session_message_t* system_message = malloc(sizeof(session_message_t));
    system_message->type = SYSTEM_MESSAGE;
    system_message->data.system.type = SESSION_UNREGISTER;
    system_message->data.system.id.session_id = session->id;
    session_bus_push(&session->bus->write, system_message);
}

void cleanup_session(session_t* session) {
    session->running = 0;
    epoll_ctl(session->epoll_fd, EPOLL_CTL_DEL, session->bus->read.event_fd, NULL);
    destroy_session_bus(session->bus);
    free(session->players);
    close(session->epoll_fd);
}

void* session_main(void* arg) {
    session_t* session = (session_t*)arg;
    struct epoll_event event;
    while (session->running) {
        const int n = epoll_wait(session->epoll_fd, &event, 1, -1);
        if (n == -1) {if (errno == EINTR) continue; perror("session: epoll_wait");break;}
        uint64_t count = 0;
        for (;;) {
            const ssize_t rd = read(session->bus->read.event_fd, &count, sizeof(count));
            if (rd == sizeof(count)) break;
            if (rd == -1 && errno == EAGAIN) break;
            if (rd == -1 && errno == EINTR) continue;
            perror("session: read(event_fd)");
            break;
        }

        ring_buffer_t* read_buffer = &session->bus->read.queue;
        while (count--) {
            session_message_t* msg = NULL;
            const int received = buffer_pop(read_buffer, &msg);
            if (received == BUFFER_EEMPTY) break;
            if (received == BUFFER_SUCCESS && msg) {
                print_session_message(msg);
                if (handle_system_message(session, msg) > 0) continue;

                // TODO handle message here

                free(msg);
            }
        }

        if (session->num_players == 0) cleanup_session_fist_step(session);
    }
    cleanup_session(session);
    free(session);
    return NULL;
}

int get_index_by_id(const session_t* session, const uint32_t user_id) {
    for (int i = 0; i < session->capacity; i++) {
        if (session->players[i].client_id == user_id) {
            return i;
        }
    }
    return -1;
}
int get_first_free_player_place(const session_t* session) {
    for (int i = 0; i < session->capacity; i++) {
        if (session->players[i].client_id == 0) {
            return i;
        }
    }
    return -1;
}