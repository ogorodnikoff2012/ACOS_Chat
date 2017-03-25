#include <ts_queue.h>
#include <stdlib.h>
#include <stdbool.h>

#define LOCK pthread_mutex_lock(&q->mutex)
#define UNLOCK pthread_mutex_unlock(&q->mutex)

void ts_queue_init(ts_queue_t *q) {
    pthread_mutex_init(&q->mutex, NULL);

    LOCK;
    q->first = NULL;
    q->last = NULL;
    q->frozen = false;
    q->size = 0;
    UNLOCK;
}

void ts_queue_destroy(ts_queue_t *q, void (* destructor)(void *)) {
    LOCK;
    while (q->first != NULL) {
        ts_queue_node_t *next = q->first->next;
        if (destructor != NULL) {
            destructor(q->first->val);
        }
        free(q->first);
        q->first = next;
    }
    q->frozen = true;
    UNLOCK;

    pthread_mutex_destroy(&q->mutex);
}

bool ts_queue_push(ts_queue_t *q, void *val) {
    bool success;
    ts_queue_node_t *node = calloc(1, sizeof(ts_queue_node_t));
    node->val = val;

    LOCK;
    if (q->frozen) {
        success = false;
    } else {
        if (q->size == 0) {
            q->last = q->first = node;
            q->size = 1;
        } else {
            q->last->next = node;
            q->last = node;
            ++q->size;
        }
        success = true;
    }
    UNLOCK;
    
    if (!success) {
        free(node);
    }
    return success;
}

void *ts_queue_pop(ts_queue_t *q) {
    void *val = NULL;
    ts_queue_node_t *node = NULL;

    LOCK;
    if (!q->frozen && q->size > 0) {
        node = q->first;
        val = node->val;
        q->first = node->next;
        --q->size;
    }
    UNLOCK;

    if (node != NULL) {
        free(node);
    }
    return val;
}

size_t ts_queue_size(ts_queue_t *q) {
    size_t size;
    LOCK;
    size = q->size;
    UNLOCK;
    return size;
}

bool ts_queue_empty(ts_queue_t *q) {
    size_t size;
    LOCK;
    size = q->size;
    UNLOCK;
    return size == 0;
}
