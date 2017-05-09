//
// Created by xenon on 5/3/17.
//

#ifndef XENON_CHAT_SERVER_CONN_MGR_H
#define XENON_CHAT_SERVER_CONN_MGR_H

#include "../event_loop/ts_map/ts_map.h"
#include <pthread.h>

typedef struct {
    ts_map_t sid_to_uid, uid_to_sid;
    pthread_mutex_t mutex;
} conn_mgr_t;

void init_conn_mgr(conn_mgr_t *mgr);
void destroy_conn_mgr(conn_mgr_t *mgr);

void conn_mgr_add_sid(conn_mgr_t *mgr, int sid);
void conn_mgr_add_uid(conn_mgr_t *mgr, int uid);
void conn_mgr_del_sid(conn_mgr_t *mgr, int sid);
void conn_mgr_del_uid(conn_mgr_t *mgr, int uid);
void conn_mgr_connect_sid_uid(conn_mgr_t *mgr, int sid, int uid);
int conn_mgr_get_uid(conn_mgr_t *mgr, int sid);
void conn_mgr_forall_uid(conn_mgr_t *mgr, int uid, void *ptr, void (* callback)(uint64_t key, void *value, void *ptr));
int conn_mgr_get_number_of_sessions(conn_mgr_t *mgr, int uid);

#endif /* XENON_CHAT_SERVER_CONN_MGR_H */
