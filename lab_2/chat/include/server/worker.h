#ifndef XENON_CHAT_SERVER_WORKER_H
#define XENON_CHAT_SERVER_WORKER_H

#include "../event_loop/event_loop.h"
#include "controller.h"
#include "../ts_vector/ts_vector.h"
#include "defines.h"
#include "listener.h"
#include "../pascal_string.h"

#define PROCESS_MESSAGE_JOB_TYPE (((WORKER_THREAD_ID) << 8) + 1)
#define SEND_MESSAGE_JOB_TYPE (((WORKER_THREAD_ID) << 8) + 2)

typedef struct {
    int worker_id;
    event_loop_t *event_loop, *controller_event_loop;
    pthread_t thread;
    controller_data_t *controller;
} worker_data_t;

typedef enum {
    DATA_C_STR,
    DATA_P_STR,
    DATA_INT32,
    DATA_INT64,
    DATA_NONE
} data_type;

typedef struct {
    union {
        char *c_str;
        uint32_t i32;
        uint64_t i64;
        pascal_string_t *p_str;
    } data;
    data_type type;
} message_token_t;

typedef struct {
    char msg_type;
    ts_vector_t *tokens;
} server_message_t;

typedef struct {
    event_t e_hdr;
    connection_t *conn;
    server_message_t *msg;
} process_message_job_t;

typedef struct {
    event_t e_hdr;
    int sid;
    uint64_t tstamp;
    char *login, *msg;
    char type;
} send_message_job_t;

void delete_server_message(server_message_t *msg);
server_message_t *new_server_message(char msg_type, ts_vector_t *tokens);
void *pack_message(server_message_t *msg);
process_message_job_t *new_process_message_job(connection_t *conn, server_message_t *msg);
send_message_job_t *new_send_message_job(int sid, uint64_t tstamp, char type, char *login, char *msg);

void workers_init(event_loop_t *event_loop);
void *worker_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_WORKER_H */
