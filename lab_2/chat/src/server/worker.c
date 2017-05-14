#include <server/worker.h>
#include <stdio.h>
#include <common/logger.h>
#include <stdlib.h>
#include <server/events/send_message_job.h>
#include <server/events/process_message_job.h>
#include <server/events/worker_exit_event.h>

void workers_init(event_loop_t *event_loop) {
    ts_map_insert(&event_loop->handlers, PROCESS_MESSAGE_JOB_TYPE, process_message_job_handler);
    ts_map_insert(&event_loop->deleters, PROCESS_MESSAGE_JOB_TYPE, process_message_job_deleter);

    ts_map_insert(&event_loop->handlers, SEND_MESSAGE_JOB_TYPE, send_message_job_handler);
}

void *worker_thread(void *ptr) {
    worker_data_t *data = (worker_data_t *) ptr;
    LOG("Worker #%d starting...", data->worker_id);
    run_event_loop(data->event_loop, data);
    LOG("Worker #%d stopping...", data->worker_id);
    send_event(data->controller_event_loop, (event_t *) new_worker_exit_event(data->worker_id));
    pthread_exit(NULL);
}
