#include "SupportStructure/int_map.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int_map_t* int_map_create(const uint16_t initial_capacity) {
    int_map_t* map = (int_map_t*)malloc(sizeof(int_map_t));
    if (!map) return NULL;

    if (initial_capacity > 0) map->capacity = initial_capacity;
    else map->capacity = 64;

    map->buffer = (int*)calloc(map->capacity, sizeof(int));
    if (!map->buffer) {free(map); return NULL;}
    return map;
}

void int_map_destroy(int_map_t* map) {
    if (map) { free(map->buffer); free(map);}
}

int int_map_set(int_map_t* map,const int key, const int value) {
    if (key < 0) return -1;

    if (key >= map->capacity) {
        const int new_capacity = (key + 1) * 2;
        int* new_buffer = realloc(map->buffer, new_capacity * sizeof(int));
        if (!new_buffer) return -1;

        // заполняем новые элементы нулями
        memset(new_buffer + map->capacity, 0, (new_capacity - map->capacity) * sizeof(int));

        map->buffer = new_buffer;
        map->capacity = new_capacity;
    }

    map->buffer[key] = value;
    return 0;
}

int int_map_get(const int_map_t* map, const int key) {
    if (key < 0 || key >= map->capacity) return 0;
    return map->buffer[key];
}

int int_map_remove(int_map_t* map, const int key) {
    if (key >= 0 && key < map->capacity) {
        map->buffer[key] = 0;
        return 0;
    }
    return -1;
}

int int_map_exists(const int_map_t* map, const int key) {
    if (key >= 0 && key < map->capacity && map->buffer[key] != 0)
        return 0;
    return -1;
}

int int_map_get_first_zero(const int_map_t* map, const int next_free_id) {
    const int bound = (next_free_id >= map->capacity) ? map->capacity - 1 : next_free_id;
    for (int i = 1; i < bound; i++) {
        if (map->buffer[i] == 0)
            return i;
    }

    return -1;
}