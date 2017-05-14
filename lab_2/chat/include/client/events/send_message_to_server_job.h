//
// Created by xenon on 13.05.17.
//

#ifndef XENON_CHAT_CLIENT_EVENTS_SEND_MESSAGE_TO_SERVER_JOB_H
#define XENON_CHAT_CLIENT_EVENTS_SEND_MESSAGE_TO_SERVER_JOB_H

#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
    char *text;
} send_message_to_server_job_t;

send_message_to_server_job_t *new_send_message_to_server_job(char *text);
void send_message_to_server_job_handler(event_t *ptr, void *dptr);
void send_message_to_server_job_deleter(event_t *ptr);

#endif //XENON_CHAT_CLIENT_EVENTS_SEND_MESSAGE_TO_SERVER_JOB_H
