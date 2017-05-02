#ifndef XENON_CHAT_SERVER_WORKER_H
#define XENON_CHAT_SERVER_WORKER_H

#include "../event_loop/event_loop.h"
#include "controller.h"
#include "../ts_vector/ts_vector.h"
#include "defines.h"
#include "listener.h"

#define PROCESS_MESSAGE_JOB_TYPE (((WORKER_THREAD_ID) << 8) + 1)

typedef struct {
    int worker_id;
    event_loop_t *event_loop, *controller_event_loop;
    pthread_t thread;
    controller_data_t *controller;
} worker_data_t;

typedef struct {
    char msg_type;
    ts_vector_t *strings;
} message_t;

typedef struct {
    event_t e_hdr;
    connection_t *conn;
    message_t *msg;
} process_message_job_t;

void delete_message(message_t *msg);
message_t *new_message(char msg_type, ts_vector_t *strings);
void *pack_message(message_t *msg);
process_message_job_t *new_process_message_job(connection_t *conn, message_t *msg);

void *worker_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_WORKER_H */
