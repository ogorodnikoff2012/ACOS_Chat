#ifndef XENON_CHAT_SERVER_WORKER_H
#define XENON_CHAT_SERVER_WORKER_H

#include "../event_loop/event_loop.h"
#include "controller.h"
#include "../ts_vector/ts_vector.h"
#include "defines.h"
#include "listener.h"
#include "../pascal_string.h"
#include "controller.h"

typedef struct worker_data {
    int worker_id;
    event_loop_t *event_loop, *controller_event_loop;
    pthread_t thread;
    controller_data_t *controller;
} worker_data_t;

void workers_init(event_loop_t *event_loop);
void *worker_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_WORKER_H */
