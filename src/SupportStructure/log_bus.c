#include "SupportStructure/log_bus.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <bits/time.h>
#include <bits/time.h>

log_bus_t* log_bus_init() {
    log_bus_t* bus = malloc(sizeof(log_bus_t));
    if (!bus) return NULL;

    if (pthread_mutex_init(&bus->mutex, NULL) != 0) { free(bus); return NULL; }
    if (pthread_cond_init(&bus->not_full, NULL) != 0) { pthread_mutex_destroy(&bus->mutex); free(bus); return NULL; }
    if (pthread_cond_init(&bus->not_empty, NULL) != 0) {
        pthread_cond_destroy(&bus->not_full);
        pthread_mutex_destroy(&bus->mutex);
        free(bus);
        return NULL;
    }

    bus->write_position = 0;
    bus->read_position = 0;
    memset(bus->entries, 0, sizeof(bus->entries));
    return bus;
}


void log_bus_destroy(log_bus_t* bus) {
    if (!bus) return;
    pthread_cond_destroy(&bus->not_empty);
    pthread_cond_destroy(&bus->not_full);
    pthread_mutex_destroy(&bus->mutex);
    free(bus);
}

int log_bus_push_blocking(log_bus_t* bus, const log_entry_t* entry) {
    pthread_mutex_lock(&bus->mutex);

    while (bus->write_position - bus->read_position >= MPSC_BUFFER_SIZE) {
        pthread_cond_wait(&bus->not_full, &bus->mutex);
    }

    const uint64_t index = bus->write_position & (MPSC_BUFFER_SIZE - 1);
    memcpy(&bus->entries[index], entry, sizeof(log_entry_t));
    bus->write_position++;

    pthread_cond_signal(&bus->not_empty);
    pthread_mutex_unlock(&bus->mutex);
    return 0;
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
    pthread_cond_signal(&bus->not_empty);
    pthread_mutex_unlock(&bus->mutex);
    return 0;
}

int log_bus_push_message(log_bus_t* bus, const uint32_t session_id, const log_level_t level, const char* message) {
    if (!bus) return -1;
    log_entry_t m;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    m.timestamp = ((uint64_t)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);

    m.session_id = session_id;
    m.level = level;
    snprintf(m.message, sizeof(m.message), "%s", message);
    if (level != LOG_SESSION) log_bus_push(bus, &m);
    else log_bus_push_blocking(bus, &m);
    return 0;
}

int log_bus_push_param(log_bus_t* bus, const uint32_t session_id, const log_level_t level, const char* message, ...) {
    if (!bus) return -1;
    char buffer[256];
    va_list args;
    va_start(args, message);
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    return log_bus_push_message(bus, session_id, level, buffer);
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

    pthread_cond_signal(&bus->not_full);
    pthread_mutex_unlock(&bus->mutex);
    return 0;
}

int log_bus_pop_blocking(log_bus_t* bus, log_entry_t* entry) {
    pthread_mutex_lock(&bus->mutex);

    while (bus->read_position >= bus->write_position) {
        pthread_cond_wait(&bus->not_empty, &bus->mutex);
        if (bus->read_position >= bus->write_position) {pthread_mutex_unlock(&bus->mutex);return -1;}
    }

    const uint64_t index = bus->read_position & (MPSC_BUFFER_SIZE - 1);
    memcpy(entry, &bus->entries[index], sizeof(log_entry_t));
    bus->read_position++;

    pthread_cond_signal(&bus->not_full);
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
        case LOG_INFO: return "INFO";
        case LOG_WARN: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_DEBUG: return "DEBUG";
        case LOG_SESSION: return "SESSION";
        default: return "UNKNOWN";
    }
}