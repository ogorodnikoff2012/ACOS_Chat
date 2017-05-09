//
// Created by xenon on 09.05.17.
//

#include <server/events/worker_exit_event.h>
#include <server/controller.h>
#include <server/worker.h>
#include <stdlib.h>

void worker_exit_event_handler(event_t *ptr, void *dptr) {
    worker_exit_event_t *evt = (worker_exit_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;

    worker_data_t *worker = ts_map_erase(&data->workers, evt->worker_id);
    if (worker != NULL) {
        kill_worker(worker);
        free(worker);
    }
}

worker_exit_event_t *new_worker_exit_event(int worker_id) {
    worker_exit_event_t *evt = calloc(1, sizeof(worker_exit_event_t));
    evt->e_hdr.type = WORKER_EXIT_EVENT_TYPE;
    evt->worker_id = worker_id;
    return evt;
}

