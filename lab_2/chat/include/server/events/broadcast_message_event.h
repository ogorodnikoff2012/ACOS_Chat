//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_BROADCAST_MESSAGE_EVENT_H
#define XENON_CHAT_SERVER_EVENTS_BROADCAST_MESSAGE_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../defines.h"

typedef struct {
    event_t e_hdr;
    uint64_t tstamp;
    char *login, *msg;
    char type;
    int uid;
} broadcast_message_event_t;

void broadcast_message_event_deleter(event_t *ptr);
void broadcast_message_callback(uint64_t key, void *val, void *env);
void broadcast_message_event_handler(event_t *ptr, void *dptr);
broadcast_message_event_t *new_broadcast_message_event(uint64_t tstamp, char type, char *login, char *msg, int uid);

#endif // XENON_CHAT_SERVER_EVENTS_BROADCAST_MESSAGE_EVENT_H
