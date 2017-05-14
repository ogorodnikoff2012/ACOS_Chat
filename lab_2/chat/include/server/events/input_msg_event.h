//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_INPUT_MSG_EVENT_H
#define XENON_CHAT_SERVER_EVENTS_INPUT_MSG_EVENT_H

#include "../../event_loop/event_loop.h"
#include "../defines.h"
#include "common/connection.h"

typedef struct {
    event_t e_hdr;
    connection_t *conn;
} input_msg_event_t;

input_msg_event_t *new_input_msg_event(connection_t *conn);
void input_msg_event_handler(event_t *ptr, void *dptr);
void input_msg_event_deleter(event_t *ptr);

#endif // XENON_CHAT_SERVER_EVENTS_INPUT_MSG_EVENT_H
