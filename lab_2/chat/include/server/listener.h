#ifndef XENON_CHAT_SERVER_LISTENER_H
#define XENON_CHAT_SERVER_LISTENER_H

#include "../event_loop/event_loop.h"
#include "conn_mgr.h"
#include "defines.h"
#include "connection.h"

typedef struct {
    event_loop_t *controller_event_loop;
    event_loop_t event_loop;
    connection_t server_socket;
    ts_map_t clients;
    conn_mgr_t *conn_mgr;
} listener_data_t;

void close_connection(listener_data_t *data, int sockid);

bool listener_init(listener_data_t *data);
void listener_destroy(listener_data_t *data);

void *listener_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_LISTENER_H */
