#include "Session/session.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#include <sys/eventfd.h>
#include <sys/timerfd.h>

int init_session(session_t* session, log_bus_t* log_bus, const uint32_t session_id, const uint8_t capacity, const uint8_t is_visible){
    log_bus_push_param(log_bus, session_id,LOG_DEBUG,"Try to init: id: %d, cap: %d, is_vis: %d",  session_id, capacity, is_visible);
    session->game = malloc(sizeof(game_t));
    if (!session->game) {log_bus_push_message(log_bus, session_id, LOG_ERROR, "Alloc error"); return -1;}
    if (init_game(session->game, 4, 5, 10, 104, 5, 20, capacity) == -1) {free(session->game); log_bus_push_message(log_bus, session_id, LOG_ERROR, "Creation error"); return -1;}

    session->max_clients = 20;
    session->game_capacity = capacity;

    session->number_clients = 0;
    session->active_players = 0;

    session->is_visible = is_visible;
    session->id = session_id;
    session->unregister_send = 0;
    session->timer_fd = -1;


    session->players = calloc(session->max_clients, sizeof(player_t));
    if (!session->players) {log_bus_push_message(log_bus, session_id, LOG_ERROR, "Alloc error session_player"); free(session->game); return -1;}

    session->bus = create_session_bus(512, session->id);
    if (!session->bus) {
        log_bus_push_message(log_bus, session_id, LOG_ERROR, "Alloc error session_bus");
        free(session->game);
        free(session->players);
        return -1;
    }
    session->log_bus = log_bus;
    session->running = 1;
    return 0;
}

void cleanup_session(session_t* session) {
    session->running = 0;
    cleanup_game(session->game);
    free(session->game);
    destroy_session_bus(session->bus);
    session->bus = NULL;
    session->log_bus = NULL;
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
        if (session->unregister_send == 1) continue;
        if (n < 0) {if (errno == EINTR) continue; log_bus_push_message(session->log_bus,session->id,LOG_ERROR,"session: poll"); break;}
        if (fds[0].revents & POLLIN) {
            uint64_t count = 0;
            for (;;) {
                const ssize_t rd = read(session->bus->read.event_fd, &count, sizeof(uint64_t));
                if (rd == sizeof(count)) break;
                if (rd == -1 && errno == EAGAIN) break;
                if (rd == -1 && errno == EINTR) continue;
                log_bus_push_message(session->log_bus,session->id,LOG_ERROR,"session read(event_fd");
                break;
            }
            ring_buffer_t* read_buffer = &session->bus->read.queue;
            while (count--) {
                session_message_t* msg = NULL;
                const int received = buffer_pop(read_buffer, &msg);
                if (received == BUFFER_EEMPTY) break;
                if (received == BUFFER_SUCCESS && msg) {
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
                if (!(rd == -1 && (errno == EAGAIN || errno == EINTR))) {
                    log_bus_push_message(session->log_bus,session->id,LOG_ERROR,"sessoin: read(timer_fd)");
                }
                else {//TODO add time related events
                    }
            }
        }

        if (session->number_clients == 0 && session->unregister_send == 0) {
            send_unregister(session);
        }
    }

    cleanup_session(session);
    free(session);
    return NULL;
}


int get_index_by_id(const session_t* session, const uint32_t user_id) {
    if (user_id == 0) return -1;
    for (size_t i = 0; i < session->max_clients; i++) {
        if (session->players[i].role != ROLE_NONE && session->players[i].player_id == user_id) {
            return (int) i;
        }
    }
    return -1;
}

int get_first_free_player_place(const session_t* session) {
    for (int i = 0; (size_t)i < session->max_clients; i++) {
        if (session->players[i].player_id == 0) {
            return (int) i;
        }
    }
    return -1;
}

int add_player(session_t* session, const uint32_t client_id, player_role_t role) {
    if (get_index_by_id(session, client_id) != -1) return -1;
    const int idx = get_first_free_player_place(session);
    if (idx == -1) return -2;
    player_t* p = &session->players[idx];

    if (role == ROLE_PLAYER && session->game->game_state != INACTIVE) {role = ROLE_OBSERVER;}
    const int card_to_alloc = (role == ROLE_PLAYER) ? session->game->nbrCardsPlayer : 0;
    create_player(p, client_id, card_to_alloc);
    p->role = role;
    p->is_connected = 1;

    session->number_clients++;
    if (role == ROLE_PLAYER) session->active_players++;
    return 0;
}

int change_player_role(session_t* session, const uint32_t player_id, const player_role_t new_role) {
    const int index = get_index_by_id(session, player_id);
    if (index == -1) return -1;

    player_t* p = &session->players[index];
    if (p->role == new_role) return 0;

    if (new_role == ROLE_PLAYER) {
        if (session->active_players >= session->game_capacity) return -2;
        if (session->game->game_state != INACTIVE) return -3;
        if (p->player_cards_id == NULL) {
            p->player_cards_id = malloc(session->game->nbrCardsPlayer * sizeof(int));
            if (p->player_cards_id == NULL) return -4;
            for (int i = 0; i < session->game->nbrCardsPlayer; i++) {p->player_cards_id[i] = -1;}
        }
        session->active_players++;
    } else if (new_role == ROLE_OBSERVER) {
        if (p->player_cards_id) {
            free(p->player_cards_id);
            p->player_cards_id = NULL;
        }
        session->active_players--;
        if (session->game->game_state != INACTIVE) {
            session->game->card_ready_to_place[index] = -1;
            const int result = start_move(session);
            handle_result_of_play(session, result); // TODO verify
        }
    }
    p->role = new_role;
    return 0;
}

int remove_player(session_t* session, const uint32_t client_id) {
    const int index = get_index_by_id(session, client_id);
    if (index == -1) return 0;
    player_t* p = &session->players[index];

    if (client_id == session->id_creator && session->number_clients > 1) {
        int new_creator_index = -1;
        for (size_t i = 0; i < session->max_clients; i++) {
            if (i == (size_t)index) continue;
            if (session->players[i].role == ROLE_PLAYER)
                new_creator_index = (int)i; break;
        }
        if (new_creator_index == -1) {
            for (size_t i = 0; i < session->max_clients; i++) {
                if (i == (size_t)index) continue;
                if (session->players[i].role == ROLE_OBSERVER)
                    new_creator_index = (int)i; break;
            }
        }
        if (new_creator_index != -1) {
            session->id_creator = session->players[new_creator_index].player_id;
        }
    }
    if (p->role == ROLE_PLAYER) {
        session->active_players--;
        if (session->game->game_state != INACTIVE) {
            session->game->card_ready_to_place[index] = -1;
            const int result = start_move(session);
            handle_result_of_play(session, result); // TODO verify
        }
    }
    session->number_clients--;
    cleanup_player(p);
    p->role = ROLE_NONE;
    p->is_connected = 0;
    return 0;
}