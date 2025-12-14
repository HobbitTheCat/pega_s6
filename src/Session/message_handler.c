#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Session/session.h"
#include "Protocol/protocol.h"

int handle_system_message(session_t* session, session_message_t* msg) {
    if (msg->type == SYSTEM_MESSAGE) {
        switch (msg->data.system.type) {
            case USER_UNREGISTER:
                remove_player(session, msg->data.system.user_id);
                const int result = start_move(session);
                handle_result_of_play(session, result);
                break;
            case USER_LEFT:
                session->left_count++;
                log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"User left left_count %lu nb player %lu",session->left_count, session->number_players);
                if (session->left_count >= session->number_players) {
                    for (int i = 0; (size_t)i < session->capacity; i++) {
                        const player_t* player = &session->players[i];
                        const uint32_t player_id = player->player_id;
                        if (player_id== 0) continue;
                        remove_player(session, player_id);
                        send_system_message_to_server(session, player_id, USER_DISCONNECTED);
                    }
                    send_unregister(session);
                }
                break;
            case SESSION_UNREGISTERED:
                session->running = 0;
                const uint64_t wake_up = 1;
                write(session->bus->read.event_fd, &wake_up, sizeof(uint64_t));
                break;
            default:
                log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Session: Unknown message type %d",msg->data.system.type);
        }
        return 1;
    }
    const uint32_t client_id = msg->data.user.client_id;
    switch (msg->data.user.packet_type) {
        case PKT_SESSION_JOIN:
            if (session->number_players == session->capacity) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Capacity exceeded");
                session_send_error_packet(session, client_id, 0x06, "Capacity exceeded"); break;}
            if (session->number_players == 0) {session->id_creator = client_id; session->unregister_send = 0;}
            if (get_index_by_id(session, client_id) != -1) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"User already registered");
                session_send_error_packet(session, client_id, 0x02, "User already registered"); break;}
            if (session->game->game_state != INACTIVE) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Game in progress please wait");
                session_send_error_packet(session, client_id, 0x08, "Game in progress please wait"); break;}
            add_player(session, client_id);
            session_send_state(session, client_id);
            send_system_message_to_server(session, client_id, USER_CONNECTED);
            break;
        case PKT_SESSION_SET_GAME_RULES:
            session_handle_set_rules(session, msg);
            break;
        case PKT_ADD_BOT:
            log_bus_push_message(session->log_bus, session->id, LOG_DEBUG, "Got bot_add packet");
            session_handle_add_bot(session, msg);
            break;
        case PKT_SESSION_LEAVE: {
            const int rc = remove_player(session, client_id);
            if (rc == 0) {
                session_send_simple_packet(session, client_id, PKT_SESSION_END);
                send_system_message_to_server(session, client_id, USER_DISCONNECTED);
            } else {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"User not in session");
                session_send_error_packet(session, client_id, 0x03, "User not in session");
            }
            const int result = start_move(session);
            handle_result_of_play(session, result);
            break;
        }
        case PKT_SESSION_CLOSE:
            if (client_id != session->id_creator) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Unauthorized access");
                session_send_error_packet(session, client_id, 0x01, "Unauthorized access"); break;}
            for (int i = 0; (size_t)i < session->capacity; i++) {
                const player_t* player = &session->players[i];
                const uint32_t player_id = player->player_id;
                if (player_id== 0) continue;
                remove_player(session, player_id);
                session_send_simple_packet(session, player_id, PKT_SESSION_END);
                send_system_message_to_server(session, player_id, USER_DISCONNECTED);
            }
            if (session->unregister_send == 0) send_unregister(session);
            return 1;
        default: return 0;
    }
    return 1;
}

int handle_result_of_play(session_t* session, const int result) {
    if (result > 0){
        game_send_request_extra(session, result);
    } else if (result == -2) {
        for (int i = 0; (size_t)i < session->capacity; i++) {
            const player_t* player = &session->players[i];
            if (player->player_id == 0) continue;
            session_send_info(session, player->player_id);
        }
    } else if (result == -3) {
        log_bus_push_message(session->log_bus,session->id,LOG_DEBUG,"Phase result:");
        for (int i = 0; (size_t)i < session->capacity; i++) {
            const player_t* player = &session->players[i];
            if (player->player_id == 0) continue;
            log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"(%d: %d)",player->player_id, player->nb_head);
            session_send_phase_result(session, player->player_id);
        }
        push_modification_log(session,PHASE_RESULT);
    } else if (result != 0)
        log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Unknown extra result %d",result);
    return 0;
}

int handle_game_message(session_t* session, const session_message_t* msg) {
    int result;

    switch (msg->data.user.packet_type) {
        case PKT_START_SESSION:
            if (msg->data.user.client_id != session->id_creator) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Unauthorized access");
                session_send_error_packet(session, msg->data.user.client_id, 0x01, "Unauthorized access");
                break;
            }
            distrib_cards(session->game, session->players, (int)session->capacity, session->log_bus, (int)session->id);
            push_modification_log(session,START);
            for (int i = 0; (size_t)i < session->capacity; i++) {
                const player_t* player = &session->players[i];
                if (player->player_id == 0) continue;
                session_send_info(session, player->player_id);
            }
            break;
        case PKT_SESSION_INFO_RETURN:
            game_handle_info_return(session, msg);
            result = start_move(session);
            handle_result_of_play(session, result);
            break;
        case PKT_RESPONSE_EXTRA:
            result = game_handle_response_extra(session, msg);
            handle_result_of_play(session, result);
            break;
        default:
            log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"Received packet: %d",msg->data.user.packet_type);
    }
    return 0;
}