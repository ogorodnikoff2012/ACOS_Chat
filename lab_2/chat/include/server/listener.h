#ifndef XENON_CHAT_SERVER_LISTENER_H
#define XENON_CHAT_SERVER_LISTENER_H

#include "../event_loop/event_loop.h"
#include <defines.h>

#define HEADER_LENGTH 5

#define INPUT_MSG_EVENT_TYPE (((LISTENER_THREAD_ID) << 8) + 1)

typedef struct {
    int sockid;
    void *header, *body;
    int bytes_read, bytes_expected;
    bool in_worker;
} connection_t;

typedef struct {
    event_loop_t *controller_event_loop;
    event_loop_t event_loop;
    connection_t server_socket;
    ts_map_t clients;
} listener_data_t;

typedef struct {
    event_t e_hdr;
    connection_t *conn;
} input_msg_event_t;

input_msg_event_t *new_input_msg_event(connection_t *conn);

connection_t *new_connection(int sockid);

bool listener_init(listener_data_t *data);
void listener_destroy(listener_data_t *data);

void *listener_thread(void *ptr);

#endif /* XENON_CHAT_SERVER_LISTENER_H */
