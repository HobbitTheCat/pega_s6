#ifndef LOG_BUS_H
#define LOG_BUS_H
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <bits/pthreadtypes.h>

#define MPSC_BUFFER_SIZE 1024

typedef enum { LOG_INFO, LOG_WARN, LOG_ERROR, LOG_DEBUG, LOG_SESSION } log_level_t;

typedef struct {
    uint64_t timestamp;
    uint32_t session_id;
    log_level_t level;
    char message[256];
} log_entry_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;

    uint64_t write_position;
    uint64_t read_position;
    log_entry_t entries[MPSC_BUFFER_SIZE];
} log_bus_t;

log_bus_t* log_bus_init();
void log_bus_destroy(log_bus_t* bus);

int log_bus_push(log_bus_t* bus, const log_entry_t* entry);
int log_bus_pop(log_bus_t* bus, log_entry_t* entry);

int log_bus_push_blocking(log_bus_t* bus, const log_entry_t* entry);
int log_bus_pop_blocking(log_bus_t* bus, log_entry_t* entry);

int log_bus_push_message(log_bus_t* bus, uint32_t session_id, log_level_t level, const char* message);
int log_bus_push_param(log_bus_t* bus, uint32_t session_id, log_level_t level, const char* message, ...);

uint64_t log_bus_mutex_available(log_bus_t* bus);

const char* log_level_name(log_level_t level);

#endif //LOG_BUS_H
