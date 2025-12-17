#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "../include/User/bot.h"
#include "../include/Protocol/protocol.h"
#include "../include/Protocol/proto_io.h"
#include "../include/User/user.h"

#define MAX_SQUADS 200
#define BOTS_PER_SQUAD 4
#define TOTAL_BOTS (MAX_SQUADS * BOTS_PER_SQUAD)
#define MAX_EVENTS 128
#define RATE_LIMIT_MS 300

typedef struct {
    uint32_t session_id;
    int ready_count;
} squad_t;

typedef enum {
    ROLE_CREATOR,
    ROLE_JOIN
} bot_role_t;

typedef enum {
    STATE_DISCONNECTED,
    STATE_PENDING_START,
    STATE_CONNECTING,
    STATE_HANDSHAKE,
    STATE_REGISTERING,
    STATE_LOBBY_WAITING,
    STATE_CREATING,
    STATE_INGAME,
    STATE_GAME_STARTED,
} bot_state_t;

typedef enum {
    TASK_NONE,
    TASK_SEND_HELLO,
    TASK_SEND_REGISTER,
    TASK_CREATE_SESSION,
    TASK_JOIN_SESSION,
    TASK_SESSION_START,
    TASK_PROCESS_GAME_INFO,
    TASK_SEND_EXTRA,
    TASK_SESSION_END
} task_type_t;

typedef struct {
    bot_t core;

    int id;
    bot_role_t role;
    bot_state_t state;
    squad_t* squad;

    uint64_t start_time;
    uint64_t last_action_time;
    task_type_t pending_task;
    uint8_t pending_payload[1024];
    uint16_t pending_payload_length;
} async_bot_t;

// TIME
uint64_t get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void schedule_task(async_bot_t* abot, const task_type_t task, const uint8_t* payload, const uint16_t length) {
    abot->pending_task = task;
    if (payload && length > 0) {
        memcpy(abot->pending_payload, payload, length);
        abot->pending_payload_length = length;
    } else abot->pending_payload_length = 0;
}

void execute_task(async_bot_t* abot) {
    bot_t* bot = &abot->core;
    switch (abot->pending_task) {
        case TASK_SEND_HELLO:
            bot_send_simple(bot, PKT_HELLO);
            abot->state = STATE_HANDSHAKE;
            break;
        case TASK_SEND_REGISTER:
            bot_send_simple(bot, PKT_REGISTER);
            abot->state = STATE_REGISTERING;
            break;
        case TASK_CREATE_SESSION:
            printf("Bot %d: Creating session\n", abot->id);
            user_send_create_session(&bot->user, BOTS_PER_SQUAD, 0);
            abot->state = STATE_CREATING;
            break;
        case TASK_JOIN_SESSION:
            printf("Bot %d: join session %d\n", abot->id, abot->squad->session_id);
            abot->core.session_id = abot->squad->session_id;
            bot_send_join_session(&abot->core);
            abot->state = STATE_INGAME;
            break;
        case TASK_SESSION_START:
            bot_send_simple(bot, PKT_START_SESSION);
            abot->state = STATE_GAME_STARTED;
            break;
        case TASK_PROCESS_GAME_INFO:
            bot_handle_session_info(bot, abot->pending_payload, abot->pending_payload_length);
            break;
        case TASK_SEND_EXTRA:
            bot_send_resp_extra(bot);
            break;
        case TASK_SESSION_END:
            printf("Session_end for %d\n", abot->id);
            bot_send_simple(bot, PKT_SESSION_LEAVE);
            bot_send_simple(bot, PKT_UNREGISTER);
            bot->user.is_running = 0;
            user_close_connection(&bot->user);
            bot_cleanup(bot);
            abot->state = STATE_DISCONNECTED;
            break;
        default:
            break;
    }
    abot->pending_task = TASK_NONE;
}


void set_nonblocking(const int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

void async_handle_packet(async_bot_t* abot, const uint8_t type, uint8_t* payload, const uint16_t length) {
    bot_t* bot = &abot->core;

    switch (type) {
        case PKT_WELCOME:
            schedule_task(abot, TASK_SEND_REGISTER, NULL, 0);
            break;
        case PKT_SYNC_STATE:
            bot_handle_sync_state(bot, payload, length);
            if (abot->role == ROLE_CREATOR) {
                schedule_task(abot, TASK_CREATE_SESSION, NULL, 0);
            } else {
                abot->state = STATE_LOBBY_WAITING;
            }
            break;
        case PKT_SESSION_STATE:
            bot_handle_session_state(bot, payload, length);
            if (abot->role == ROLE_CREATOR) {
                printf("Joined session with id %d\n", bot->session_id);
                abot->squad->session_id = bot->session_id;
            }
            abot->squad->ready_count++;
            abot->state = STATE_INGAME;
            break;
        case PKT_SESSION_INFO:
            schedule_task(abot, TASK_PROCESS_GAME_INFO, payload, length);
            break;
        case PKT_REQUEST_EXTRA:
            schedule_task(abot, TASK_SEND_EXTRA, NULL, 0);
            break;
        case PKT_PHASE_RESULT:
            printf("got Phase result\n");
        case PKT_SESSION_END:
            schedule_task(abot, TASK_SESSION_END, NULL, 0);
            break;
        case PKT_ERROR:
            printf("Bot %d: Error \n", abot->id);
            break;
        default: printf("Bot %d: Received packet \n", abot->id);
    }
}

int main() {
    int is_running = 1;
    const char* host = "127.0.0.1";
    const int port = 17001;

    const int epoll_fd = epoll_create1(0);
    struct epoll_event events[MAX_EVENTS];

    squad_t* squads = calloc(MAX_SQUADS, sizeof(squad_t));
    if (!squads) return -1;
    async_bot_t* bots = calloc(TOTAL_BOTS, sizeof(async_bot_t));
    if (!bots) {free(squads); return -1;}

    printf("Initializing %d bots (%d squads)\n", TOTAL_BOTS, MAX_SQUADS);

    const uint64_t base = get_time();
    const int delta = 50;
    for (int i = 0; i < TOTAL_BOTS; i++) {
        bots[i].id = i;
        bots[i].role = (i % BOTS_PER_SQUAD == 0) ? ROLE_CREATOR : ROLE_JOIN;
        bots[i].squad = &squads[i / BOTS_PER_SQUAD];

        bot_init(&bots[i].core);
        bots[i].state = STATE_PENDING_START;
        bots[i].core.level = 0;
        bots[i].start_time = base + (uint64_t)i * delta;
        bots[i].last_action_time = base;
    }

    printf("Starting Event Loop.\n");

    while (is_running) {
        const int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 50);
        const uint64_t now = get_time();


        for (int i = 0; i < n; i++) {
            async_bot_t* abot = (async_bot_t*)events[i].data.ptr;
            const uint32_t event = events[i].events;

            if (event & (EPOLLERR | EPOLLHUP)) {
                close(abot->core.user.sockfd);
                abot->state = STATE_DISCONNECTED;
                continue;
            }

            if (abot->state == STATE_CONNECTING && (event & EPOLLOUT)) {
                schedule_task(abot, TASK_SEND_HELLO, NULL, 0);
                continue;
            }

            if (event & EPOLLIN) {
                user_t* user = &abot->core.user;
                ssize_t received = recv(user->sockfd, user->rx.buf + user->rx.have, sizeof(user->rx.buf) - user->rx.have, 0);
                if (received > 0) {
                    user->rx.have += received;
                    while (1) {
                        header_t header;
                        uint8_t* payload;
                        uint16_t payload_length;
                        const int res = rx_try_extract_frame(&user->rx, &header, &payload, &payload_length);
                        if (res != 1) break;
                        async_handle_packet(abot, header.type, payload, payload_length);
                    }
                } else if (received == 0) {
                    close(user->sockfd);
                    abot->state = STATE_DISCONNECTED;
                }
            }
            if (event & EPOLLOUT) bot_handle_write(&abot->core);
        }

        int inactive_count = 0;
        for (int i = 0; i < TOTAL_BOTS; i++) {
            async_bot_t* abot = &bots[i];


            if (abot->state == STATE_DISCONNECTED) {inactive_count+=1; continue;}

            if (abot->state == STATE_PENDING_START && now >= abot->start_time) {
                bot_t* bot = &abot->core;
                bot->user.sockfd = socket(AF_INET, SOCK_STREAM, 0);
                set_nonblocking(bot->user.sockfd);

                struct sockaddr_in addr = {0};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port);
                inet_pton(AF_INET, host, &addr.sin_addr);

                connect(bots[i].core.user.sockfd, (struct sockaddr*)&addr, sizeof(addr));
                bots[i].state = STATE_CONNECTING;
                bots[i].last_action_time = now;

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
                ev.data.ptr = abot;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, bot->user.sockfd, &ev);

                continue;
            }

            if (abot->role == ROLE_JOIN && abot->state == STATE_LOBBY_WAITING && abot->pending_task == TASK_NONE &&
                abot->squad->session_id > 0) {
                schedule_task(abot, TASK_JOIN_SESSION, NULL, 0);
            }

            if (abot->role == ROLE_CREATOR && abot->state == STATE_INGAME && abot->pending_task == TASK_NONE &&
                abot->squad->ready_count >= BOTS_PER_SQUAD) {
                schedule_task(abot, TASK_SESSION_START, NULL, 0);
            }

            if (abot->pending_task != TASK_NONE) {
                if (now >= abot->last_action_time + RATE_LIMIT_MS) {
                    execute_task(abot);
                    abot->last_action_time = now;
                }
            }
        }
        if (inactive_count == TOTAL_BOTS) is_running = 0;
    }
    free(squads);
    free(bots);
    return 0;
}
