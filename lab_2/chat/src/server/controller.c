#include <server/controller.h>
#include <server/worker.h>
#include <stdlib.h>
#include <logger.h>
#include <unistd.h>
#include <server/events/broadcast_message_event.h>
#include <server/misc.h>
#include <server/events/input_msg_event.h>
#include <server/events/worker_exit_event.h>
#include <server/events/change_worker_cnt_event.h>

void decrease_workers(controller_data_t *data) {
    send_urgent_event(&data->workers_event_loop, &EXIT_EVT);
}

void kill_worker(worker_data_t *data) {
    pthread_join(data->thread, NULL);
    --data->controller->workers_cnt;
}

void run_worker(controller_data_t *data) {
    worker_data_t *worker = calloc(1, sizeof(worker_data_t));
    worker->controller = data;
    worker->event_loop = &data->workers_event_loop;
    worker->controller_event_loop = &data->event_loop;
    worker->worker_id = unique_id();
    pthread_create(&worker->thread, NULL, worker_thread, worker);
    ts_map_insert(&data->workers, worker->worker_id, worker);
    ++data->workers_cnt;
}

int controller_init(controller_data_t *data) {
    data->workers_cnt = 0;
    ts_map_init(&data->workers);
    event_loop_init(&data->event_loop);
    event_loop_init(&data->workers_event_loop);
    workers_init(&data->workers_event_loop);

    ts_map_insert(&data->event_loop.handlers, CHANGE_WORKER_CNT_EVENT_TYPE, change_worker_cnt_event_handler);
    ts_map_insert(&data->event_loop.handlers, WORKER_EXIT_EVENT_TYPE, worker_exit_event_handler);

    ts_map_insert(&data->event_loop.handlers, INPUT_MSG_EVENT_TYPE, input_msg_event_handler);
    ts_map_insert(&data->event_loop.deleters, INPUT_MSG_EVENT_TYPE, input_msg_event_deleter);

    ts_map_insert(&data->event_loop.handlers, BROADCAST_MESSAGE_EVENT_TYPE, broadcast_message_event_handler);
    ts_map_insert(&data->event_loop.deleters, BROADCAST_MESSAGE_EVENT_TYPE, broadcast_message_event_deleter);

    init_conn_mgr(&data->conn_mgr);
    int rc = sqlite3_open(DB_FILENAME, &data->db);
    return rc;
}

void controller_destroy(controller_data_t *data) {
    destroy_conn_mgr(&data->conn_mgr);
    ts_map_destroy(&data->workers, (void (*)(void *)) kill_worker);
    if (data->db != NULL) {
        int rc;
        while ((rc = sqlite3_close(data->db)) != SQLITE_OK) {
            usleep(100000); /* 100 ms */
        }
    }
    event_loop_destroy(&data->workers_event_loop);
    event_loop_destroy(&data->event_loop);
}

void *controller_thread(void *ptr) {
    LOG("Starting controller...");
    controller_data_t *data = (controller_data_t *) ptr;
    run_event_loop(&data->event_loop, data);

    LOG("Stopping workers...");
    for (int i = 0; i < data->workers_cnt; ++i) {
        decrease_workers(data);
    }
    
    ts_map_erase(&data->event_loop.handlers, CHANGE_WORKER_CNT_EVENT_TYPE);
    ts_map_erase(&data->event_loop.handlers, INPUT_MSG_EVENT_TYPE);
    while (data->workers_cnt > 0) {
        event_loop_iteration(&data->event_loop, data);
        usleep(10000); /* 10 ms */
    }
    
    LOG("Stopping controller...");
    pthread_exit(NULL);
}
