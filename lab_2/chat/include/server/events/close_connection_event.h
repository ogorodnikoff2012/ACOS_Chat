//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_CLOSE_CONNECTION_EVENT_H
#define XENON_CHAT_SERVER_EVENTS_CLOSE_CONNECTION_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../defines.h"

typedef struct {
    event_t e_hdr;
    int sockid;
} close_connection_event_t;

close_connection_event_t *new_close_connection_event(int sockid);
void close_connection_event_handler(event_t *ptr, void *dptr);


#endif // XENON_CHAT_SERVER_EVENTS_CLOSE_CONNECTION_EVENT_H
