//
// Created by xenon on 14.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_STATUS_EVENT_H
#define XENON_CHAT_CLIENT_EVENTS_STATUS_EVENT_H

#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
    int status;
} status_event_t;

status_event_t *new_status_event(int status);
void status_event_handler(event_t *ptr, void *dptr);

#endif // XENON_CHAT_CLIENT_EVENTS_STATUS_EVENT_H
