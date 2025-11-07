#ifndef FD_MAP_H
#define FD_MAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void **buffer;
    uint16_t capacity;
} fd_map_t;

fd_map_t *fd_map_create(uint16_t initial_capacity);
void fd_map_destroy(fd_map_t* map);

int fd_map_set(fd_map_t* map, int fd, void* ptr);
void* fd_map_get(fd_map_t* map, int fd);
void fd_map_remove(fd_map_t* map, int fd);

int fd_map_exists(fd_map_t* map, int fd);

#endif //FD_MAP_H
