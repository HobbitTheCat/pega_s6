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
void cleanup_log_thread(log_thread* log_thread) {
    log_thread->running = 0;
    log_bus_destroy(log_thread->log_bus);
}

void* log_thread_loop(void* arg) {
    log_thread* self = (log_thread*)arg;
    FILE* file = fopen(self->path, "a");
    if (file == NULL) {
        perror("Erreur: Ouverture FAIL");
        return NULL;
    }
    log_entry_t msg;

    while (self->running) {
        const int result = log_bus_pop(self->log_bus, &msg);
        if (result == 0) {
            fprintf(file, "[%lu] [%s] [ID:%u] %s\n",msg.timestamp,log_level_name(msg.level),msg.session_id,msg.message);
            fflush(file);
        }
        else {
            sleep(1);
        }
    }
    fclose(file);
    return NULL;
}