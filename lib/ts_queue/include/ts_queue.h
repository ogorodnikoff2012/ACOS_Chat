#ifndef XENON_TS_QUEUE_TS_QUEUE_H
#define XENON_TS_QUEUE_TS_QUEUE_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct ts_queue_node {
    struct ts_queue_node *next;
    void *val;
} ts_queue_node_t;

typedef struct ts_queue {
    ts_queue_node_t *first, *last;
    bool frozen;
    pthread_mutex_t mutex;
    size_t size;
} ts_queue_t;

void ts_queue_init(ts_queue_t *q);
void ts_queue_destroy(ts_queue_t *q, void (* destructor)(void *));

bool ts_queue_push(ts_queue_t *q, void *val);
bool ts_queue_push_in_front(ts_queue_t *q, void *val);
void *ts_queue_pop(ts_queue_t *q);
size_t ts_queue_size(ts_queue_t *q);
bool ts_queue_empty(ts_queue_t *q);

#endif /* XENON_TS_QUEUE_TS_QUEUE_H */
