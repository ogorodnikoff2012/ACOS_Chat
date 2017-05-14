//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_EVENTS_PROCESS_MESSAGE_JOB_H
#define XENON_CHAT_SERVER_EVENTS_PROCESS_MESSAGE_JOB_H

#include "../defines.h"
#include "../../event_loop/event_loop.h"
#include "../../common/message.h"
#include "common/connection.h"

typedef struct {
    event_t e_hdr;
    connection_t *conn;
    message_t *msg;
} process_message_job_t;

void process_message_job_deleter(event_t *ptr);
void process_message_job_handler(event_t *ptr, void *dptr);
process_message_job_t *new_process_message_job(connection_t *conn, message_t *msg);

#endif // XENON_CHAT_SERVER_EVENTS_PROCESS_MESSAGE_JOB_H
