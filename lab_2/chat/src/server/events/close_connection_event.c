//
// Created by xenon on 09.05.17.
//

#include <server/events/close_connection_event.h>
#include <server/listener.h>
#include <stdlib.h>

void close_connection_event_handler(event_t *ptr, void *dptr) {
    listener_data_t *data = dptr;
    close_connection_event_t *evt = (close_connection_event_t *) ptr;
    close_connection(data, evt->sockid);
}

close_connection_event_t *new_close_connection_event(int sockid) {
    close_connection_event_t *evt = calloc(1, sizeof(close_connection_event_t));
    evt->e_hdr.type = CLOSE_CONNECTION_EVENT_TYPE;
    evt->sockid = sockid;
    return evt;
}

