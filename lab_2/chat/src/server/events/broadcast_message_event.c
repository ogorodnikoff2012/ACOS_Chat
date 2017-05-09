//
// Created by xenon on 09.05.17.
//

#include <server/events/broadcast_message_event.h>
#include <server/events/send_message_job.h>
#include <stdlib.h>
#include <string.h>
#include <server/controller.h>
#include <pair.h>

void broadcast_message_event_deleter(event_t *ptr) {
    broadcast_message_event_t *evt = (broadcast_message_event_t *) ptr;
    free(evt->login);
    free(evt->msg);
    free(evt);
}

void broadcast_message_callback(uint64_t key, void *val, void *env) {
    controller_data_t *data = ((pair_t *) env)->second;
    broadcast_message_event_t *evt = ((pair_t *) env)->first;
    send_event(&data->workers_event_loop,
               (event_t *) new_send_message_job(key, evt->tstamp, evt->type,
                                                evt->login == NULL ? NULL : strdup(evt->login), strdup(evt->msg)));
}

void broadcast_message_event_handler(event_t *ptr, void *dptr) {
    broadcast_message_event_t *evt = (broadcast_message_event_t *) ptr;
    controller_data_t *data = dptr;

    if (evt->type == MESSAGE_SERVER_REGULAR) {
        int rc;
        const char *tail;
        sqlite3_stmt *stmt;
        sqlite3_prepare(data->db, "INSERT INTO messages VALUES(?001, ?002, ?003);", -1, &stmt, &tail);
        sqlite3_bind_int(stmt, 1, evt->uid);
        sqlite3_bind_text(stmt, 2, evt->msg, -1, SQLITE_STATIC);
        sqlite3_bind_int64(stmt, 3, evt->tstamp);

        while ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (rc == SQLITE_ERROR) {
                break;
            }
        }
        sqlite3_finalize(stmt);
    }

    pair_t pair;
    pair.first = evt;
    pair.second = data;
    ts_map_forall(&data->conn_mgr.sid_to_uid, &pair, broadcast_message_callback);
}

broadcast_message_event_t *new_broadcast_message_event(uint64_t tstamp, char type, char *login, char *msg, int uid) {
    broadcast_message_event_t *evt = calloc(1, sizeof(broadcast_message_event_t));
    evt->e_hdr.type = BROADCAST_MESSAGE_EVENT_TYPE;
    evt->tstamp = tstamp;
    evt->msg = msg;
    evt->login = login;
    evt->uid = uid;
    evt->type = type;

    return evt;
}

