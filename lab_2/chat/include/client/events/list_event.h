//
// Created by xenon on 14.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_LIST_EVENT_H
#define XENON_CHAT_CLIENT_EVENTS_LIST_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../../common/message.h"

typedef struct {
    event_t e_hdr;
    message_t *msg;
} list_event_t;

list_event_t *new_list_event(message_t *msg);
void list_event_handler(event_t *ptr, void *dptr);
void list_event_deleter(event_t *ptr);

#endif // XENON_CHAT_CLIENT_EVENTS_LIST_EVENT_H
