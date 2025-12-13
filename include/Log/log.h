#ifndef LOG_H
#define LOG_H

#include "SupportStructure/log_bus.h"

typedef struct {
    _Atomic int running;

    log_bus_t* log_bus;
    char* path;
} log_thread;

int init_log_thread(log_thread* log_thread, char* path);
void cleanup_log_thread(log_thread* log_thread);

void* log_thread_loop(void* arg);
#endif //LOG_H
