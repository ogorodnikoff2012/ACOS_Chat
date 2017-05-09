//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_SEND_MESSAGE_JOB_H
#define XENON_CHAT_SERVER_EVENTS_SEND_MESSAGE_JOB_H

#include "../defines.h"
#include "../../event_loop/event_loop.h"

typedef struct {
    event_t e_hdr;
    int sid;
    uint64_t tstamp;
    char *login, *msg;
    char type;
} send_message_job_t;

void send_message_job_handler(event_t *ptr, void *dptr);
send_message_job_t *new_send_message_job(int sid, uint64_t tstamp, char type, char *login, char *msg);

#endif // XENON_CHAT_SERVER_EVENTS_SEND_MESSAGE_JOB_H
