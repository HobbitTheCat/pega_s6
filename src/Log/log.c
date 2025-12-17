#include "Log/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int init_log_thread(log_thread* log_thread, char* path) {
    remove(path);
    log_thread->log_bus = log_bus_init();
    if (log_thread->log_bus == NULL) {return -1;}
    log_thread->path = path;
    log_thread->running = 1;
    return 0;
}

void log_bus_stop_waiting(log_bus_t* bus) {
    pthread_mutex_lock(&bus->mutex);
    pthread_cond_broadcast(&bus->not_empty);
    pthread_mutex_unlock(&bus->mutex);
}

void cleanup_log_thread(log_thread* log_thread, const pthread_t thread) {
    log_thread->running = 0;
    log_bus_stop_waiting(log_thread->log_bus);
    pthread_join(thread, NULL);
    log_bus_destroy(log_thread->log_bus);
}

void* log_thread_loop(void* arg) {
    const log_thread* self = (log_thread*)arg;
    FILE* file = fopen(self->path, "a");
    if (file == NULL) return NULL;

    log_entry_t msg;

    while (self->running) {
        const int res = log_bus_pop_blocking(self->log_bus, &msg);
        if (res == 0) {
            fprintf(file, "[%lu] [%s] [ID:%u] %s\n",msg.timestamp,log_level_name(msg.level),msg.session_id,msg.message);
            fflush(file);
        }
        else {}
    }

    while (log_bus_pop(self->log_bus, &msg) == 0) {
        fprintf(file, "[%lu] [%s] [ID:%u] %s\n", msg.timestamp, log_level_name(msg.level), msg.session_id, msg.message);
    }

    fclose(file);
    return NULL;
}