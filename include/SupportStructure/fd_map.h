#ifndef FD_MAP_H
#define FD_MAP_H

#include <stdint.h>

typedef struct {
    void **buffer;
    uint16_t capacity;
} fd_map_t;

fd_map_t* fd_map_create(uint16_t initial_capacity);
void fd_map_destroy(fd_map_t* map);

int fd_map_set(fd_map_t* map, int fd, void* ptr);
void* fd_map_get(const fd_map_t* map, int fd);
void fd_map_remove(const fd_map_t* map, int fd);

int fd_map_exists(const fd_map_t* map, int fd);
int fd_map_get_first_null_id(const fd_map_t* map, int next_free_id);

#endif //FD_MAP_H
