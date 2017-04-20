#ifndef XENON_CHAT_SERVER_WORKER_H
#define XENON_CHAT_SERVER_WORKER_H

#include <sqlite3.h>
#include "../event_loop/event_loop.h"
#include "controller.h"

typedef struct {
    int worker_id;
    event_loop_t *event_loop, *controller_event_loop;
    sqlite3 *db;
    pthread_t thread;
    controller_data_t *controller;
} worker_data_t;

void *worker_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_WORKER_H */
