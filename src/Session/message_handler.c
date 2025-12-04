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
                remove_player(session, msg->data.system.user_id); break;
            case SESSION_UNREGISTERED:
                session->running = 0;
                const uint64_t wake_up = 1;
                write(session->bus->read.event_fd, &wake_up, sizeof(uint64_t));
                break;
            default:
                printf("Session: Unknown message type %d\n", msg->data.system.type);
        }
        return 1;
    }
    const uint32_t client_id = msg->data.user.client_id;
    switch (msg->data.user.packet_type) {
        case PKT_SESSION_JOIN:
            if (session->number_players == session->capacity) {
                session_send_error_packet(session, client_id, 0x06, "Capacity exceeded"); break;}
            if (session->number_players == 0) {session->id_creator = client_id; session->unregister_send = 0;}
            if (get_index_by_id(session, client_id) != -1) {
                session_send_error_packet(session, client_id, 0x02, "User already registered"); break;}
            add_player(session, client_id);
            session_send_simple_packet(session, client_id, PKT_SESSION_STATE);
            send_system_message_to_server(session, client_id, USER_CONNECTED);
            break;
        case PKT_SESSION_LEAVE: {
            const int rc = remove_player(session, client_id);
            if (rc == 0) {
                session_send_simple_packet(session, client_id, PKT_SESSION_END);
                send_system_message_to_server(session, client_id, USER_DISCONNECTED);
            } else
                session_send_error_packet(session, client_id, 0x03, "User not in session");
            break;
        }
        case PKT_SESSION_CLOSE:
            if (client_id != session->id_creator) {
                session_send_error_packet(session, client_id, 0x01, "Unauthorized access"); break;}
            for (int i = 0; (size_t)i < session->capacity; i++) {
                const player_t* player = &session->players[i];
                if (player->player_id == 0) continue;
                remove_player(session, player->player_id);
                session_send_simple_packet(session, client_id, PKT_SESSION_END);
                send_system_message_to_server(session, client_id, USER_DISCONNECTED);
            }
            send_unregister(session);
            return 1;
        default: return 0;
    }
    return 1;
}