//
// Created by xenon on 09.05.17.
//

#include <server/events/input_msg_event.h>
#include <server/controller.h>
#include <server/listener.h>
#include <netinet/in.h>
#include <logger.h>
#include <ts_vector/ts_vector.h>
#include <server/server_message.h>
#include <pascal_string.h>
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

    int msglen = ntohl(*(int *)(header + 1));
    LOG("Found message: type \'%c\', length %d", header[0], msglen);

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    char *iter = body, *end = body + msglen;

    while (iter < end) {
        pascal_string_t *str = (pascal_string_t *) iter;
        str->length = ntohl(str->length);
        char *next = iter + 4 + str->length;
        if (next > end) {
            LOG("Message corrupted");
            evt->conn->in_worker = false;
            send_status_code(evt->conn->sockid, MSG_STATUS_INVALID_MSG);
            for (int i = 0; i < tokens->size; ++i) {
                free(((message_token_t *) tokens->data)[i].data.p_str);
            }
            ts_vector_destroy(tokens);
            free(tokens);
            return;
        }
        message_token_t token;
        token.type = DATA_P_STR;
        token.data.p_str = pstrdup(str);
        ts_vector_push_back(tokens, &token);
        iter = next;
    }

    LOG("End of message");
    send_event(&data->workers_event_loop,
               (event_t *) new_process_message_job(evt->conn,
                                                   new_server_message(header[0], tokens)));
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

