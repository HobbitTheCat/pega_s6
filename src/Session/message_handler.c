#include <stdio.h>

#include "Session/session.h"
#include "Protocol/protocol.h"

#include <stdlib.h>
#include <unistd.h>

int send_user_response(const session_t* session, const session_message_t* msg, const uint16_t packet_size, const uint8_t* buffer) {
    return send_user_message(&session->bus->write, msg->data.user.client_id, msg->data.user.client_fd,
                    msg->data.user.session_id, 0, packet_size, buffer);
}


int handle_system_message(session_t* session, session_message_t* msg) {
    uint8_t* buffer = malloc(PROTOCOL_MAX_SIZE);
    if (!buffer) return -1;
    int packet_size = 0;
    if (msg->type == SYSTEM_MESSAGE) {
        if (msg->data.system.type == CLIENT_UNREGISTER) {
            remove_player(session, msg->data.system.id.client_id);
        } else if (msg->data.system.type == SESSION_UNREGISTERED) {
            free(buffer);
            session->running = 0;
            const uint64_t wake_up = 1;
            write(session->bus->read.event_fd, &wake_up, sizeof(wake_up));
            return 1;
        } else {
            printf("Unhandled message type %d\n", msg->data.system.type);
        }
        free(buffer); return 1;
    } else {
        switch (msg->data.user.packet_type) {
            case PKT_SESSION_JOIN:
                if (session->num_players == session->capacity) {
                    packet_size = create_error_packet(buffer, session->id, msg->data.user.client_id, 0x06, "Capacity exceeded");
                    break;
                } if (session->num_players == 0) {
                    session->id_creator = msg->data.user.client_id;
                } else {
                    if (get_index_by_id(session, msg->data.user.client_id) != -1) {create_error_packet(buffer, session->id, msg->data.user.session_id, 0x02, "User already registered"); break;}
                }
                player_t player;
                create_player(&player, msg->data.user.client_fd, msg->data.user.client_id);
                session->players[get_first_free_player_place(session)] = player;
                session->num_players++;
                packet_size = create_simple_packet(buffer, PKT_SESSION_STATE, session->id, msg->data.user.client_id);
                break;
            case PKT_SESSION_LEAVE:
                if (remove_player(session, msg->data.user.client_id) == -1) {
                    packet_size = create_error_packet(buffer, session->id, msg->data.user.client_id, 0x03, "Invalid user index"); break;
                }
                packet_size = create_simple_packet(buffer, PKT_SESSION_END, session->id, msg->data.user.client_id);
                break;
            case PKT_SESSION_CLOSE:
                if (msg->data.user.client_id != session->id_creator) {packet_size = create_error_packet(buffer, session->id, msg->data.user.client_id, 0x01, "Unauthorized access"); break;}
                for (int i = 0; i < session->capacity; i++) {
                    const player_t* player_to_send = &session->players[i];
                    if (player_to_send->client_id == 0) continue;
                    packet_size = create_simple_packet(buffer, PKT_SESSION_END, session->id, player_to_send->client_id);
                    send_user_message(&session->bus->write, player_to_send->client_id, player_to_send->fd, session->id, 0, packet_size, buffer);
                }
                free(buffer);
                cleanup_session_fist_step(session);
                return 1;
            default:
                free(buffer);
                return 0;
        }
    }
    if (packet_size > 0) send_user_response(session, msg, packet_size, buffer);
    free(buffer);
    return 1;
}