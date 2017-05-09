#ifndef XENON_CHAT_SERVER_CONRTOLLER_H
#define XENON_CHAT_SERVER_CONRTOLLER_H

#include "../event_loop/event_loop.h"
#include "defines.h"
#include "conn_mgr.h"

#include <sqlite3.h>

typedef struct worker_data worker_data_t;

typedef struct {
    event_loop_t event_loop, workers_event_loop, *listener_event_loop;
    ts_map_t workers;
    int workers_cnt;
    sqlite3 *db;
    conn_mgr_t conn_mgr;
} controller_data_t;

void decrease_workers(controller_data_t *data);
void kill_worker(worker_data_t *data);
void run_worker(controller_data_t *data);

int controller_init(controller_data_t *data);
void controller_destroy(controller_data_t *data);
void *controller_thread(void *ptr);

// void register_user(const char *login, const char *passwd);

#endif /* XENON_CHAT_SERVER_CONRTOLLER_H */
