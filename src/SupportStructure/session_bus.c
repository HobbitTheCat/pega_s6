#include <sys/eventfd.h>
#include <unistd.h>
#include <stdlib.h>

#include "SupportStructure/session_bus.h"

#include <stdio.h>

session_bus_t* create_session_bus(const uint32_t capacity, const uint32_t session_id) {
    session_bus_t* session_bus = malloc(sizeof(*session_bus));
    if (!session_bus) return NULL;

    session_bus->session_id = session_id;
    session_bus->read_capacity = capacity;
    session_bus->write_capacity = capacity;

    session_bus->read.event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (session_bus->read.event_fd < 0) {free(session_bus);return NULL;}

    session_bus->write.event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (session_bus->write.event_fd < 0) {
        close(session_bus->read.event_fd);
        free(session_bus);
        return NULL;
    }

    if (buffer_init(&session_bus->read.queue, capacity, sizeof(session_message_t*)) != BUFFER_SUCCESS) {
        close(session_bus->read.event_fd);
        close(session_bus->write.event_fd);
        free(session_bus);
        return NULL;
    }
    if (buffer_init(&session_bus->write.queue, capacity, sizeof(session_message_t*)) != BUFFER_SUCCESS) {
        buffer_destroy(&session_bus->read.queue);
        close(session_bus->read.event_fd);
        close(session_bus->write.event_fd);
        free(session_bus);
        return NULL;
    }

    return session_bus;
}

void destroy_session_bus(session_bus_t *session_bus) {
    if (!session_bus) return;

    close(session_bus->read.event_fd);
    close(session_bus->write.event_fd);

    buffer_destroy(&session_bus->read.queue);
    buffer_destroy(&session_bus->write.queue);

    free(session_bus);
}

int session_bus_push(bus_base_t* bus, session_message_t* message) {
    const int result = buffer_push(&bus->queue, &message);
    if (result != BUFFER_SUCCESS) return result;
    const uint64_t one = 1;
    write(bus->event_fd, &one, sizeof(one));
    return BUFFER_SUCCESS;
}

void print_session_message(const session_message_t *msg) {
    if (!msg) return;
    printf("┌────────────────────────────┐\n");
    if (msg->type == SYSTEM_MESSAGE) {
        printf("│     New system message:    │\n");
        if (msg->data.system.type == USER_DISCONNECTED) {
            printf("│    Client unregistered     │\n");
            printf("├────────────────────────────┤\n");
            printf("│Client ID: %u               │\n", msg->data.system.user_id);
        } else if (msg->data.system.type == USER_CONNECTED) {
            printf("│    Client registered    │\n");
            printf("├────────────────────────────┤\n");
            printf("│Session ID: %u               │\n", msg->data.system.session_id);
        } else {
            printf("│    Session unregistered    │\n");
            printf("├────────────────────────────┤\n");
            printf("│Session ID: %u               │\n", msg->data.system.session_id);
        }
    } else {
        printf("│      New user message      │\n");
        printf("├────────────────────────────┤\n");
        printf("│Client ID: %u                │\n", msg->data.user.client_id);
        printf("│Packet type: %u              │\n", msg->data.user.packet_type);
        printf("│Payload length: %u          │\n", msg->data.user.payload_length);
    }
    printf("└────────────────────────────┘\n");
}