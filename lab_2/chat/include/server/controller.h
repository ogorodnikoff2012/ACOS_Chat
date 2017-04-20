#ifndef XENON_CHAT_SERVER_CONRTOLLER_H
#define XENON_CHAT_SERVER_CONRTOLLER_H

#include "../event_loop/event_loop.h"
#include "../defines.h"
#include <sqlite3.h>

#define WORKER_EXIT_EVENT_TYPE (((CONTROLLER_THREAD_ID) << 8) + 1)
#define CHANGE_WORKER_CNT_EVENT_TYPE (((CONTROLLER_THREAD_ID) << 8) + 2)

typedef struct {
    event_loop_t event_loop, workers_event_loop;
    ts_map_t workers;
    int workers_cnt;
    sqlite3 *db;
} controller_data_t;

typedef struct {
    event_t e_hdr;
    int worker_id;
} worker_exit_event_t;

typedef struct {
    event_t e_hdr;
    int cnt;
} change_worker_cnt_event_t;

worker_exit_event_t *new_worker_exit_event(int worker_id);
change_worker_cnt_event_t *new_change_worker_cnt_event_t(int cnt);

int controller_init(controller_data_t *data);
void controller_destroy(controller_data_t *data);
void *controller_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_CONRTOLLER_H */
