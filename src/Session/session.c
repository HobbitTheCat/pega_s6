#include "Session/session.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#include <sys/eventfd.h>
#include <sys/timerfd.h>

int init_session(session_t* session, const uint32_t session_id, const uint8_t capacity, const uint8_t is_visible){
    printf("Try to init: id: %d, cap: %d, is_vis: %d\n", session_id, capacity, is_visible);
    session->game = malloc(sizeof(game_t));
    if (!session->game) {printf("!game\n"); return -1;}
    if (init_game(session->game, 4, 5, 10, 104, 5, capacity) == -1) {free(session->game); printf("!init_game\n"); return -1;}
    session->capacity = capacity;
    session->number_players = 0;
    session->is_visible = is_visible;
    session->id = session_id;
    session->unregister_send = 1;
    session->timer_fd = -1;


    session->players = calloc(capacity, sizeof(player_t));
    if (!session->players) {perror("session: create session bus"); free(session->game); return -1;}

    session->bus = create_session_bus(256, session->id);
    if (!session->bus) {
        perror("session: create session bus");
        free(session->game);
        free(session->players);
        return -1;
    }

    session->running = 1;
    return 0;
}

void cleanup_session(session_t* session) {
    session->running = 0;
    cleanup_game(session->game);
    free(session->game);
    destroy_session_bus(session->bus);
    session->bus = NULL;
    free(session->players);
    session->players = NULL;
    if (session->timer_fd != -1) {
        close(session->timer_fd);
        session->timer_fd = -1;
    }
}

void send_unregister(session_t* session) {
    session_message_t* system_message = malloc(sizeof(session_message_t));
    system_message->type = SYSTEM_MESSAGE;
    system_message->data.system.type = SESSION_UNREGISTER;
    system_message->data.system.session_id = session->id;
    if (session_bus_push(&session->bus->write, system_message) == -1) {free(system_message);
        session->unregister_send = 0;};
    session->unregister_send = 1;
}

void* session_main(void* arg) {
    session_t* session = (session_t*)arg;
    struct pollfd fds[2];
    nfds_t nfds = 1;

    fds[0].fd = session->bus->read.event_fd;
    fds[0].events = POLLIN;

    if (session->timer_fd != -1) {
        fds[1].fd = session->timer_fd;
        fds[1].events = POLLIN;
        nfds = 2;
    }


    while (session->running) {
        const int n = poll(fds, nfds, -1);
        if (n < 0) {if (errno == EINTR) continue; perror("session: poll"); break;}
        if (fds[0].revents & POLLIN) {
            uint64_t count = 0;
            for (;;) {
                const ssize_t rd = read(session->bus->read.event_fd, &count, sizeof(uint64_t));
                if (rd == sizeof(count)) break;
                if (rd == -1 && errno == EAGAIN) break;
                if (rd == -1 && errno == EINTR) continue;
                perror("session read(event_fd");
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
                    handle_game_message(session, msg);

                    free(msg);
                }
            }
        }

        if (nfds == 2 && (fds[1].revents & POLLIN)) {
            uint64_t expiration;
            const ssize_t rd = read(session->timer_fd, &expiration, sizeof(uint64_t));
            if (rd == sizeof(expiration)) {
                if (!(rd == -1 && (errno == EAGAIN || errno == EINTR))) {perror("sessoin: read(timer_fd)");}
                else {//TODO add time related events
                    }
            }
        }

        if (session->number_players == 0 && session->unregister_send == 0) {
            send_unregister(session);
        }
    }

    cleanup_session(session);
    free(session);
    return NULL;
}

int get_index_by_id(const session_t* session, const uint32_t user_id) {
    for (int i = 0; (size_t)i < session->capacity; i++) {
        if (session->players[i].player_id == user_id) {
            return i;
        }
    }
    return -1;
}
int get_first_free_player_place(const session_t* session) {
    for (int i = 0; (size_t)i < session->capacity; i++) {
        if (session->players[i].player_id == 0) {
            return i;
        }
    }
    return -1;
}

int add_player(session_t* session, const uint32_t client_id) {
    if (get_index_by_id(session, client_id) != -1) return -1;
    const int idx = get_first_free_player_place(session);
    if (idx == -1) return -2;
    player_t* p = &session->players[idx];
    create_player(p, client_id, session->game->nbrCardsPlayer);
    session->number_players++;
    return 0;
}

int remove_player(session_t* session, const uint32_t client_id) {
    const int player_index = get_index_by_id(session, client_id);
    if (player_index == -1) return -1;
    if (client_id == session->id_creator && session->number_players > 1)
        for (int i = 0; (size_t)i < session->capacity; i++)
            if (session->players[i].player_id != 0) {session->id_creator = session->players[i].player_id; break;}
    cleanup_player(&session->players[player_index]);
    session->number_players--;
    return 0;
}