//
// Created by xenon on 14.05.17.
//

#include <client/events/list_event.h>
#include <stdlib.h>
#include <client/defines.h>
#include <client/controller.h>
#include <common/message.h>
#include <ts_vector/ts_vector.h>
#include <stdio.h>
#include <common/pascal_string.h>
#include <netinet/in.h>
#include <client/events/display_parsed_message_job.h>
#include <client/misc.h>

list_event_t *new_list_event(message_t *msg) {
    list_event_t *evt = calloc(1, sizeof(list_event_t));
    evt->e_hdr.type = LIST_EVENT_TYPE;
    evt->msg = msg;
    return evt;
}

void list_event_handler(event_t *ptr, void *dptr) {
    client_controller_data_t *data = dptr;
    list_event_t *evt = (list_event_t *) ptr;

    message_token_t *tokens = evt->msg->tokens->data;
    for (int i = 0; i < evt->msg->tokens->size; i += 2) {
        char *msg = NULL;
        char *user = p_str_to_c_str(tokens[i + 1].data.p_str);
        uint32_t id = ntohl(*(uint32_t *) tokens[i].data.p_str->data);
        asprintf(&msg, "User \"%s\", id = %d", user, id);
        send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
                MESSAGE_SERVER_LIST, NULL, msg, get_tstamp()
        )));
        free(user);
    }
}

void list_event_deleter(event_t *ptr) {
    list_event_t *evt = (list_event_t *) ptr;
    delete_message(evt->msg);
    free(evt);
}
