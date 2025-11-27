#ifndef INT_MAP_H
#define INT_MAP_H
#include <stdint.h>

typedef struct {
    int* buffer;
    uint16_t capacity;
} int_map_t;

int_map_t* int_map_create(uint16_t initial_capacity);
void int_map_destroy(int_map_t* map);

int int_map_set(int_map_t* map, int key, int value);
int int_map_get(const int_map_t* map, int key);
int int_map_remove(int_map_t* map, int key);

int int_map_exists(const int_map_t* map, int key);
int int_map_get_first_zero(const int_map_t* map, int next_free_id);

#endif //INT_MAP_H
