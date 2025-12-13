#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "../include/Log/log.h"
#include "../include/SupportStructure/log_bus.h"

int main() {
    log_thread my_logger;
    char* filename = "test.log";
    remove(filename);

    if (init_log_thread(&my_logger, filename) != 0) {fprintf(stderr, "Error of init"); return -1; }
    pthread_t thread_for_log;
    if (pthread_create(&thread_for_log, NULL, log_thread_loop, &my_logger) != 0) {fprintf(stderr, "Error of pthread_create"); return -1; }

    printf("Log thread start\n");
    for (int i = 0; i < 5; i++) {
        log_entry_t message;

        message.timestamp = (uint64_t) time(NULL);
        message.session_id = 100 + i;

        if (i%2 == 0) {
            message.level = LOG_INFO;
            snprintf(message.message, sizeof(message.message), "Hello World");
        } else {
            message.level = LOG_WARN;
            snprintf(message.message, sizeof(message.message), "Attention World");
        }

        int res = log_bus_push(my_logger.log_bus, &message);
        if (res != 0) {printf("Error message not out\n");}
        sleep(1);
    }
    printf("Log thread stopped\n");
    my_logger.running = 0;
    sleep(1);
    cleanup_log_thread(&my_logger);
    return 0;
}