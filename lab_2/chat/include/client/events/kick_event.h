//
// Created by xenon on 14.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_KICK_EVENT_H
#define XENON_CHAT_CLIENT_EVENTS_KICK_EVENT_H

#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
    char *reason;
} kick_event_t;

kick_event_t *new_kick_event(char *str);
void kick_event_handler(event_t *ptr, void *dptr);
void kick_event_deleter(event_t *ptr);

#endif // XENON_CHAT_CLIENT_EVENTS_KICK_EVENT_H
