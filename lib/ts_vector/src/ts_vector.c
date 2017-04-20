#include <ts_vector.h>
#include <stdlib.h>
#include <string.h>

#define LOCK pthread_mutex_lock(&v->mutex)
#define UNLOCK pthread_mutex_unlock(&v->mutex)

#define INITIAL_CAPACITY 8
 
void ts_vector_init(ts_vector_t *v, size_t elem_size) {
    pthread_mutex_init(&v->mutex, NULL);

    LOCK;
    v->elem_size = elem_size;
    v->size = 0;
    v->capacity = INITIAL_CAPACITY;
    v->data = malloc(v->capacity * elem_size);
    UNLOCK;
}

void ts_vector_destroy(ts_vector_t *v) {
    LOCK;
    free(v->data);
    UNLOCK;
    pthread_mutex_destroy(&v->mutex);
}

static void grow_up(ts_vector_t *v) {
    void *ptr = realloc(v->data, v->capacity * v->elem_size * 2);
    if (ptr != NULL) {
        v->data = ptr;
        v->capacity *= 2;
    }
}

void ts_vector_push_back(ts_vector_t *v, void *val) {
    LOCK;
    if (v->size == v->capacity) {
        grow_up(v);
    }
    memcpy(v->data + v->elem_size * v->size, val, v->elem_size);
    ++v->size;
    UNLOCK;
}

void ts_vector_clear(ts_vector_t *v) {
    LOCK;
    v->size = 0;
    UNLOCK;
}

