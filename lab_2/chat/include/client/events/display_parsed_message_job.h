//
// Created by xenon on 13.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_DISPLAY_PARSED_MESSAGE_JOB_H
#define XENON_CHAT_CLIENT_EVENTS_DISPLAY_PARSED_MESSAGE_JOB_H

#include "../../event_loop/event_loop.h"
#include "../parsed_message.h"

typedef struct {
    event_t e_hdr;
    parsed_message_t *msg;
} display_parsed_message_job_t;

display_parsed_message_job_t *new_display_parsed_message_job(parsed_message_t *msg);
void display_parsed_message_job_handler(event_t *ptr, void *dptr);
void display_parsed_message_job_deleter(event_t *ptr);


#endif // XENON_CHAT_CLIENT_EVENTS_DISPLAY_PARSED_MESSAGE_JOB_H
