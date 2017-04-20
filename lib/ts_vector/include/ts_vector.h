#ifndef XENON_TS_VECTOR_TS_VECTOR_H
#define XENON_TS_VECTOR_TS_VECTOR_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void *data;
    size_t elem_size, size, capacity;
    pthread_mutex_t mutex;
} ts_vector_t;

void ts_vector_init(ts_vector_t *v, size_t elem_size);
void ts_vector_destroy(ts_vector_t *v);

void ts_vector_push_back(ts_vector_t *v, void *val);
void ts_vector_clear(ts_vector_t *v);

#endif /* XENON_TS_VECTOR_TS_VECTOR_H */

