//
// Created by xenon on 09.05.17.
//

#include <server/events/input_msg_event.h>
#include <server/controller.h>
#include <server/listener.h>
#include <netinet/in.h>
#include <common/logger.h>
#include <ts_vector/ts_vector.h>
#include <common/message.h>
#include <common/pascal_string.h>
#include <server/events/process_message_job.h>
#include <server/misc.h>
#include <stdlib.h>

input_msg_event_t *new_input_msg_event(connection_t *conn) {
    input_msg_event_t *evt = calloc(1, sizeof(input_msg_event_t));
    evt->e_hdr.type = INPUT_MSG_EVENT_TYPE;
    evt->conn = conn;
    return evt;
}

void input_msg_event_handler(event_t *ptr, void *dptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    controller_data_t *data = (controller_data_t *) dptr;
    char *header = evt->conn->header;
    char *body = evt->conn->body;

    message_t *msg = unpack_message_2(header, body);
    if (msg == NULL) {
        evt->conn->in_worker = false;
        send_status_code(evt->conn->sockid, MSG_STATUS_INVALID_MSG);
    } else {
        send_event(&data->workers_event_loop, (event_t *) new_process_message_job(evt->conn, msg));
    }
}

void input_msg_event_deleter(event_t *ptr) {
    input_msg_event_t *evt = (input_msg_event_t *) ptr;
    free(evt->conn->header);
    free(evt->conn->body);
    evt->conn->bytes_read = 0;
    evt->conn->bytes_expected = MSG_HEADER_SIZE;
    evt->conn->header = NULL;
    evt->conn->body = NULL;
    free(evt);
}

