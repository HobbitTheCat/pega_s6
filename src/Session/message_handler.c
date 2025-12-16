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
                break;
            case USER_LEFT:
                const int index = get_index_by_id(session, msg->data.system.user_id);
                if (index == -1) break;
                player_t* player = &session->players[index];
                change_player_role(session, msg->data.system.user_id, ROLE_OBSERVER);
                player->is_connected = 0;

                int number_of_rest = 0;
                for (size_t i = 0; i < session->max_clients; i++)
                    if (session->players[i].is_connected == 1) number_of_rest++;

                log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"User left remain %lu nb player %lu",number_of_rest, session->number_clients);
                if (number_of_rest == 0) {
                    for (size_t i = 0; i < session->max_clients; i++) {
                        const uint32_t player_id = session->players[i].player_id;
                        if (player_id == 0) continue;
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
                log_bus_push_param(session->log_bus,session->id,LOG_WARN,"Session: Unknown message type %d",msg->data.system.type);
        }
        return 1;
    }
    const uint32_t client_id = msg->data.user.client_id;
    int result = 0;
    switch (msg->data.user.packet_type) {
        case PKT_SESSION_JOIN:
            if (session->number_clients >= session->max_clients) {
                log_bus_push_message(session->log_bus, session->id, LOG_WARN, "Server full");
                session_send_error_packet(session, client_id, 0x06, "Server full"); break; }
            if (get_index_by_id(session, client_id) != -1) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"User already registered");
                session_send_error_packet(session, client_id, 0x02, "User already registered"); break; }
            if (session->number_clients == 0) { session->id_creator = client_id; session->unregister_send =0;}
            player_role_t target_role = ROLE_OBSERVER;
            if (session->game->game_state == INACTIVE && session->active_players < session->game_capacity) {target_role = ROLE_PLAYER;}
            result = add_player(session, client_id, target_role);
            if (result == 0) {
                session_send_state(session, client_id);
                if (target_role == ROLE_OBSERVER && session->game->game_state != INACTIVE) {
                    session_send_info(session, client_id);
                }
                send_system_message_to_server(session, client_id, USER_CONNECTED);
                log_bus_push_param(session->log_bus, session->id, LOG_DEBUG, "Client %d joined as %s", client_id, (target_role == ROLE_PLAYER ? "PLAYER" : "OBSERVER"));
            } else {
                log_bus_push_param(session->log_bus, session->id, LOG_ERROR, "Failed to add player with code %d", result);
                session_send_error_packet(session, client_id, 0x06, "Internal error joining session");
            } break;
        case PKT_SESSION_SET_GAME_RULES:
            session_handle_set_rules(session, msg);
            break;
        case PKT_ADD_BOT:
            if (session->game->game_state != INACTIVE) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Try to add bot but session already started");
                session_send_error_packet(session, client_id, 0x08, "Session already started");
            } else {
                log_bus_push_message(session->log_bus, session->id, LOG_DEBUG, "Got bot_add packet");
                session_handle_add_bot(session, msg);
            }
            break;
        case PKT_SESSION_LEAVE: {
            result = remove_player(session, client_id);
            if (result == 0) {
                session_send_simple_packet(session, client_id, PKT_SESSION_END);
                send_system_message_to_server(session, client_id, USER_DISCONNECTED);
            } else {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"User not in session");
                session_send_error_packet(session, client_id, 0x03, "User not in session");
            } break;
        }
        case PKT_SESSION_CLOSE:
            if (client_id != session->id_creator) {
                log_bus_push_message(session->log_bus,session->id,LOG_WARN,"Unauthorized access");
                session_send_error_packet(session, client_id, 0x01, "Unauthorized access"); break;}
            log_bus_push_message(session->log_bus, session->id, LOG_DEBUG, "Got session close packet");
            session->game->game_state = INACTIVE;
            for (size_t i = 0; i < session->max_clients; i++) {
                const uint32_t player_id = session->players[i].player_id;
                if (player_id == 0) continue;
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
        for (int i = 0; (size_t)i < session->max_clients; i++) {
            const player_t* player = &session->players[i];
            if (player->player_id == 0) continue;
            session_send_info(session, player->player_id);
        }
    } else if (result == -3) {
        log_bus_push_message(session->log_bus,session->id,LOG_DEBUG,"Phase result:");
        for (int i = 0; (size_t)i < session->max_clients; i++) {
            const player_t* player = &session->players[i];
            if (player->player_id == 0) continue;
            log_bus_push_param(session->log_bus,session->id,LOG_DEBUG,"(%d: %d)",player->player_id, player->nb_head);
            session_send_phase_result(session, player->player_id);
        }
        session->game->game_state = INACTIVE;
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
            distrib_cards(session->game, session->players, (int)session->max_clients, session->log_bus, (int)session->id);
            push_modification_log(session,START);
            for (int i = 0; (size_t)i < session->max_clients; i++) {
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