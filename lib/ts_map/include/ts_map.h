#ifndef XENON_TS_MAP_TS_MAP_H
#define XENON_TS_MAP_TS_MAP_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>

typedef struct ts_map_node {
    struct ts_map_node *left, *right, *parent;
    int key;
    void *val;
} ts_map_node_t;

typedef struct ts_map {
    ts_map_node_t *root;
    pthread_mutex_t mutex;
} ts_map_t;

void ts_map_init(ts_map_t *m);
void ts_map_destroy(ts_map_t *m);

void ts_map_insert(ts_map_t *m, int key, void *val);
void *ts_map_find(ts_map_t *m, int key);
void ts_map_erase(ts_map_t *m, int key);

#endif /* XENON_TS_MAP_TS_MAP_H */
