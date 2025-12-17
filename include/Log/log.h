#ifndef LOG_H
#define LOG_H

#include "SupportStructure/log_bus.h"

typedef struct {
    _Atomic int running;

    log_bus_t* log_bus;
    char* path;
} log_thread;

void log_bus_stop_waiting(log_bus_t* bus);

int init_log_thread(log_thread* log_thread, char* path);
void cleanup_log_thread(log_thread* log_thread, pthread_t thread);

void* log_thread_loop(void* arg);
#endif //LOG_H
