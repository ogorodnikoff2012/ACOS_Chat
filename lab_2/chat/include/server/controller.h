#ifndef XENON_CHAT_SERVER_CONRTOLLER_H
#define XENON_CHAT_SERVER_CONRTOLLER_H

#include "../event_loop/event_loop.h"
#include "../defines.h"
#include "conn_mgr.h"

#include <sqlite3.h>

#define WORKER_EXIT_EVENT_TYPE (((CONTROLLER_THREAD_ID) << 8) + 1)
#define CHANGE_WORKER_CNT_EVENT_TYPE (((CONTROLLER_THREAD_ID) << 8) + 2)
#define BROADCAST_MESSAGE_EVENT_TYPE (((CONTROLLER_THREAD_ID) << 8) + 3)

typedef struct {
    event_loop_t event_loop, workers_event_loop, *listener_event_loop;
    ts_map_t workers;
    int workers_cnt;
    sqlite3 *db;
    conn_mgr_t conn_mgr;
} controller_data_t;

typedef struct {
    event_t e_hdr;
    int worker_id;
} worker_exit_event_t;

typedef struct {
    event_t e_hdr;
    int cnt;
} change_worker_cnt_event_t;

typedef struct {
    event_t e_hdr;
    uint64_t tstamp;
    char *login, *msg;
    char type;
    int uid;
} broadcast_message_event_t;

worker_exit_event_t *new_worker_exit_event(int worker_id);
change_worker_cnt_event_t *new_change_worker_cnt_event(int cnt);
broadcast_message_event_t *new_broadcast_message_event(uint64_t tstamp, char type, char *login, char *msg, int uid);

int controller_init(controller_data_t *data);
void controller_destroy(controller_data_t *data);
void *controller_thread(void *ptr);

void send_status_code(int sockid, int status);
void register_user(const char *login, const char *passwd);

#endif /* XENON_CHAT_SERVER_CONRTOLLER_H */
