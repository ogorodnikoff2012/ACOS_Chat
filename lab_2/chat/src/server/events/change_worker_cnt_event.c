//
// Created by xenon on 09.05.17.
//

#include <server/events/change_worker_cnt_event.h>
#include <server/controller.h>
#include <stdlib.h>

void change_worker_cnt_event_handler(event_t *ptr, void *dptr) {
    change_worker_cnt_event_t *evt = (change_worker_cnt_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;
    int delta = evt->cnt - data->workers_cnt;
    while (delta > 0) {
        run_worker(data);
        --delta;
    }
    while (delta < 0) {
        decrease_workers(data);
        ++delta;
    }
}

change_worker_cnt_event_t *new_change_worker_cnt_event(int cnt) {
    change_worker_cnt_event_t *evt = calloc(1, sizeof(change_worker_cnt_event_t));
    evt->e_hdr.type = CHANGE_WORKER_CNT_EVENT_TYPE;
    evt->cnt = cnt;
    return evt;
}

