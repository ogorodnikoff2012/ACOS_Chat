//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_WORKER_EXIT_EVENT_H
#define XENON_CHAT_SERVER_EVENTS_WORKER_EXIT_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../defines.h"

typedef struct {
    event_t e_hdr;
    int worker_id;
} worker_exit_event_t;

worker_exit_event_t *new_worker_exit_event(int worker_id);
void worker_exit_event_handler(event_t *ptr, void *dptr);

#endif // XENON_CHAT_SERVER_EVENTS_WORKER_EXIT_EVENT_H
