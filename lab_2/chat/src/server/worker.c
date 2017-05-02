#include <server/worker.h>
#include <server/controller.h>
#include <pthread.h>
#include <stdio.h>
#include <logger.h>
#include <stdlib.h>
#include <ts_vector/ts_vector.h>
#include <netinet/in.h>
#include <server/listener.h>

void delete_message(message_t *msg) {
    for (int i = 0; i < msg->strings->size; ++i) {
        free(*((char **)(msg->strings->data) + i));
    }
    ts_vector_destroy(msg->strings);
    free(msg->strings);
    free(msg);
}

void *pack_message(message_t *msg) {
    uint32_t size = MSG_HEADER_SIZE;
    for (int i = 0; i < msg->strings->size; ++i) {
        char *str = ((char **)(msg->strings->data))[i];
        size += sizeof(uint32_t) + strlen(str);
    }
    void *buffer = malloc(size);
    void *it = buffer;

    *(char *)it = msg->msg_type;
    it += sizeof(char);
    *(uint32_t *)it = htonl(size - MSG_HEADER_SIZE);
    it += sizeof(uint32_t);

    for (int i = 0; i < msg->strings->size; ++i) {
        char *str = ((char **)(msg->strings->data))[i];
        uint32_t length = strlen(str);
        *(uint32_t *)it = htonl(length);
        it += sizeof(uint32_t);
        while (length--) {
            *(char *)it++ = *str++;
        }
    }
    return buffer;
}

static void process_message_job_deleter(event_t *ptr) {
    process_message_job_t *job = (process_message_job_t *) ptr;
    delete_message(job->msg);
    job->conn->in_worker = false;
    free(job);
}

static void process_message_job_handler(event_t *ptr, void *dptr) {
    process_message_job_t *job = (process_message_job_t *) ptr;
    worker_data_t *data = (worker_data_t *) dptr;
    LOG("Worker #%d is processing a message...", data->worker_id);
    switch (job->msg->msg_type) {
        case 'r':

            break;
        default:
            send_status_code(job->conn->sockid, MSG_STATUS_INVALID_TYPE);
    }
}

message_t *new_message(char msg_type, ts_vector_t *strings) {
    message_t *msg = calloc(1, sizeof(message_t));
    msg->msg_type = msg_type;
    msg->strings = strings;
    return msg;
}

process_message_job_t *new_process_message_job(connection_t *conn, message_t *msg) {
    process_message_job_t *job = calloc(1, sizeof(process_message_job_t));
    job->e_hdr.type = PROCESS_MESSAGE_JOB_TYPE;
    job->msg = msg;
    job->conn = conn;
    return job;
}

void *worker_thread(void *ptr) {
    worker_data_t *data = (worker_data_t *) ptr;
    LOG("Worker #%d starting...", data->worker_id);

    ts_map_insert(&data->event_loop->handlers, PROCESS_MESSAGE_JOB_TYPE, process_message_job_handler);
    ts_map_insert(&data->event_loop->deleters, PROCESS_MESSAGE_JOB_TYPE, process_message_job_deleter);

    run_event_loop(data->event_loop, data);
    LOG("Worker #%d stopping...", data->worker_id);
    send_event(data->controller_event_loop, (void *) new_worker_exit_event(data->worker_id));
    pthread_exit(NULL);
}
