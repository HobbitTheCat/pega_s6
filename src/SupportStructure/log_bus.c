#include "SupportStructure/log_bus.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


log_bus_t* log_bus_init() {
    log_bus_t* bus = malloc(sizeof(log_bus_t));
    const int ret = pthread_mutex_init(&bus->mutex, NULL);
    if (ret != 0) {return NULL;}

    bus->write_position = 0;
    bus->read_position = 0;
    memset(bus->entries, 0, sizeof(bus->entries));
    return bus;
}

void log_bus_destroy(log_bus_t* bus) {
    pthread_mutex_destroy(&bus->mutex);
    free(bus);
}

int log_bus_push(log_bus_t* bus, const log_entry_t* entry) {
    pthread_mutex_lock(&bus->mutex);

    if (bus->write_position - bus->read_position >= MPSC_BUFFER_SIZE) {
        pthread_mutex_unlock(&bus->mutex);
        return -1;
    }

    const uint64_t index = bus->write_position & (MPSC_BUFFER_SIZE - 1);
    memcpy(&bus->entries[index], entry, sizeof(log_entry_t));
    bus->write_position++;
    pthread_mutex_unlock(&bus->mutex);
    return 0;
}

int log_bus_push_message(log_bus_t* bus, const uint32_t session_id, const log_level_t level, const char* message) {
    log_entry_t m;

    m.timestamp = (uint64_t) time(NULL);
    m.session_id = session_id;
    m.level = level;
    snprintf(m.message, sizeof(m.message), message);

    return log_bus_push(bus, &m);
}

int log_bus_pop(log_bus_t* bus, log_entry_t* entry) {
    pthread_mutex_lock(&bus->mutex);
    if (bus->read_position >= bus->write_position) {
        pthread_mutex_unlock(&bus->mutex);
        return -1;
    }
    const uint64_t index = bus->read_position & (MPSC_BUFFER_SIZE - 1);
    memcpy(entry, &bus->entries[index], sizeof(log_entry_t));
    bus->read_position++;
    pthread_mutex_unlock(&bus->mutex);
    return 0;
}

uint64_t log_bus_mutex_available(log_bus_t* bus) {
    pthread_mutex_lock(&bus->mutex);
    const uint64_t count = bus->write_position - bus->read_position;
    pthread_mutex_unlock(&bus->mutex);
    return count;
}

const char* log_level_name(const log_level_t level) {
    switch (level) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        default:        return "UNKNOWN";
    }
}