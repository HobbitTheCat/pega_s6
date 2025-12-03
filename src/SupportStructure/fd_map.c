#include "SupportStructure/fd_map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>


fd_map_t *fd_map_create(const uint16_t initial_capacity) {
    fd_map_t *map = (fd_map_t *)malloc(sizeof(fd_map_t));
    if (!map) return NULL;

    if (initial_capacity > 0)
        map->capacity = initial_capacity;
    else
        map->capacity = 64;
    map->buffer = (void**)calloc(map->capacity, sizeof(void *));
    if (!map->buffer) {free(map); return NULL;}
    return map;
}

int fd_map_set(fd_map_t* map, const int fd, void* ptr) {
    if (fd < 0) return -1;

    if (fd >= map->capacity) {
        const int new_capacity = (fd + 1)*2;
        void **new_buffer = realloc(map->buffer, new_capacity * sizeof(void *));
        if (!new_buffer) return -1;

        memset(new_buffer + map->capacity, 0, (new_capacity - map->capacity)*sizeof(void*));

        map->buffer = new_buffer;
        map->capacity = new_capacity;
    }

    map->buffer[fd] = ptr;
    return 0;
}

void* fd_map_get(const fd_map_t* map, const int fd) {
    if (fd < 0 || fd >= map->capacity) return NULL;
    return map->buffer[fd];
}

void fd_map_remove(const fd_map_t* map, const int fd) {
    if (fd > 0 && fd < map->capacity) {
        map->buffer[fd] = NULL;
    }
}

int fd_map_exists(const fd_map_t* map, const int fd) {
    if (fd >= 0 && fd < map->capacity && map->buffer[fd] != NULL) {
        return 0;
    } return -1;
}

void fd_map_destroy(fd_map_t* map) {
    if (map) {
        free(map->buffer);
        free(map);
    }
}

int fd_map_get_first_null_id(const fd_map_t* map, const int next_free_id) { // TODO check if this works
    int bound;
    if (next_free_id >= map->capacity) bound = map->capacity-1;
    else bound = next_free_id;

    for (int i = 1; i < bound; i++) {
        if (map->buffer[i] == NULL) return i;
    }
    return -1;
}