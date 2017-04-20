#include <server/worker.h>
#include <server/controller.h>
#include <pthread.h>
#include <stdio.h>

void *worker_thread(void *ptr) {
    worker_data_t *data = (worker_data_t *) ptr;
    printf("Worker #%d starting...\n", data->worker_id);
    run_event_loop(data->event_loop, data);
    printf("Worker #%d stopping...\n", data->worker_id);
    send_event(data->controller_event_loop, (void *) new_worker_exit_event(data->worker_id));
    pthread_exit(NULL);
}
