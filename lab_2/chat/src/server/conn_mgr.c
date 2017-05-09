//
// Created by xenon on 5/3/17.
//

#include <server/conn_mgr.h>
#include <stdlib.h>

#define LOCK pthread_mutex_lock(&mgr->mutex)
#define UNLOCK pthread_mutex_unlock(&mgr->mutex)

void init_conn_mgr(conn_mgr_t *mgr) {
    pthread_mutex_init(&mgr->mutex, NULL);

    LOCK;
    ts_map_init(&mgr->sid_to_uid);
    ts_map_init(&mgr->uid_to_sid);
    UNLOCK;
}

static void map_destroyer(void *ptr) {
    ts_map_t *m = ptr;
    ts_map_destroy(m, NULL);
    free(m);
}

void destroy_conn_mgr(conn_mgr_t *mgr) {
    LOCK;
    ts_map_destroy(&mgr->sid_to_uid, NULL);
    ts_map_destroy(&mgr->uid_to_sid, map_destroyer);
    UNLOCK;
    pthread_mutex_destroy(&mgr->mutex);
}

void conn_mgr_add_sid(conn_mgr_t *mgr, int sid) {
    LOCK;
    ts_map_insert(&mgr->sid_to_uid, sid, 0);
    UNLOCK;
}

void conn_mgr_add_uid(conn_mgr_t *mgr, int uid) {
    LOCK;
    ts_map_t *m = calloc(1, sizeof(ts_map_t));
    ts_map_init(m);
    if (!ts_map_insert(&mgr->uid_to_sid, uid, m)) {
        ts_map_destroy(m, NULL);
        free(m);
    }
    UNLOCK;
}

void conn_mgr_del_sid(conn_mgr_t *mgr, int sid) {
    LOCK;
    int uid = (int)(uint64_t) ts_map_erase(&mgr->sid_to_uid, sid);
    ts_map_t *m = ts_map_find(&mgr->uid_to_sid, uid);
    if (m != NULL) {
        ts_map_erase(m, sid);
    }
    UNLOCK;
}

static void del_uid_helper(uint64_t key, void *val, void *ptr) {
    conn_mgr_t *mgr = ptr;
    ts_map_erase(&mgr->sid_to_uid, key);
}

void conn_mgr_del_uid(conn_mgr_t *mgr, int uid) {
    LOCK;
    ts_map_t *m = ts_map_erase(&mgr->uid_to_sid, uid);
    if (m != NULL) {
        ts_map_forall(m, mgr, del_uid_helper);
        ts_map_destroy(m, NULL);
        free(m);
    }
    UNLOCK;
}

void conn_mgr_connect_sid_uid(conn_mgr_t *mgr, int sid, int uid) {
    LOCK;
    if (ts_map_has(&mgr->sid_to_uid, sid)) {
        int old_uid = (int)(uint64_t) ts_map_erase(&mgr->sid_to_uid, sid);
        ts_map_t *m = ts_map_find(&mgr->uid_to_sid, old_uid);
        if (m != NULL) {
            ts_map_erase(m, sid);
        }

        ts_map_insert(&mgr->sid_to_uid, sid, (void *)(uint64_t) uid);
        m = ts_map_find(&mgr->uid_to_sid, uid);
        if (m != NULL) {
            ts_map_insert(m, sid, NULL);
        }
    }
    UNLOCK;
}

int conn_mgr_get_uid(conn_mgr_t *mgr, int sid) {
    int uid;
    LOCK;
    uid = (int)(uint64_t) ts_map_find(&mgr->sid_to_uid, sid);
    UNLOCK;
    return uid;
}

void conn_mgr_forall_uid(conn_mgr_t *mgr, int uid, void *ptr, void (* callback)(uint64_t key, void *value, void *ptr)) {
    LOCK;
    ts_map_t *m = ts_map_find(&mgr->uid_to_sid, uid);
    if (m != NULL) {
        ts_map_forall(m, ptr, callback);
    }
    UNLOCK;
}

int conn_mgr_get_number_of_sessions(conn_mgr_t *mgr, int uid) {
    int ans = 0;
    LOCK;
    ts_map_t *m = ts_map_find(&mgr->uid_to_sid, uid);
    if (m != NULL && m->root != NULL) {
        ans = m->root->size;
    }
    UNLOCK;
    return ans;
}
