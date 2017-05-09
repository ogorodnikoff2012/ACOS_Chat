#ifndef XENON_TS_MAP_TS_MAP_H
#define XENON_TS_MAP_TS_MAP_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct ts_map_node {
    struct ts_map_node *left, *right, *parent;
    uint64_t key;
    void *val;
    size_t size;
} ts_map_node_t;

typedef struct ts_map {
    ts_map_node_t *root;
    pthread_mutex_t mutex;
    bool frosen;
} ts_map_t;

void ts_map_init(volatile ts_map_t *m);
void ts_map_destroy(volatile ts_map_t *m, void (* destructor)(void *));

bool ts_map_insert(volatile ts_map_t *m, uint64_t key, void *val);
void *ts_map_find(volatile ts_map_t *m, uint64_t key);
bool ts_map_has(volatile ts_map_t *m, uint64_t key);
void *ts_map_erase(volatile ts_map_t *m, uint64_t key);

size_t ts_map_size(volatile ts_map_t *m);

void ts_map_forall(volatile ts_map_t *m, void *ptr, void (* callback)(uint64_t key, void *value, void *ptr));

#endif /* XENON_TS_MAP_TS_MAP_H */
