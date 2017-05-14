//
// Created by xenon on 13.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_READINPUT_EVENT_H
#define XENON_CHAT_CLIENT_EVENTS_READINPUT_EVENT_H

#include <stdbool.h>
#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
    char *prompt;
    bool passwd;
    void *env;
    void (*callback_success)(char *, void *);
    void (*callback_error)(void *);
} readinput_event_t;

readinput_event_t *new_readinput_event(char *prompt, bool passwd, void *env,
                                       void (*callback_success)(char *, void *),
                                       void (*callback_error)(void *));
void readinput_event_handler(event_t *ptr, void *dptr);
void readinput_event_deleter(event_t *ptr);


#endif // XENON_CHAT_CLIENT_EVENTS_READINPUT_EVENT_H
