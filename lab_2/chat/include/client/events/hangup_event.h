//
// Created by xenon on 14.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_HANGUP_EVENT_H
#define XENON_CHAT_CLIENT_EVENTS_HANGUP_EVENT_H

#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
} hangup_event_t;

hangup_event_t *new_hangup_event();
void hangup_event_handler(event_t *ptr, void *dptr);
void hangup_event_deleter(event_t *ptr);

#endif // XENON_CHAT_CLIENT_EVENTS_HANGUP_EVENT_H
