//
// Created by xenon on 14.05.17.
//

#include <client/events/status_event.h>
#include <stdlib.h>
#include <client/controller.h>
#include <stdio.h>
#include <client/defines.h>
#include <client/misc.h>
#include <client/events/display_parsed_message_job.h>
#include <pair.h>

status_event_t *new_status_event(int status) {
    status_event_t *evt = calloc(1, sizeof(status_event_t));
    evt->e_hdr.type = STATUS_EVENT_TYPE;
    evt->status = status;
    return evt;
}

void status_event_handler(event_t *ptr, void *dptr) {
    client_controller_data_t *data = dptr;
    status_event_t *evt = (status_event_t *) ptr;

    if (!data->logged_in) {
        if (evt->status == MSG_STATUS_OK) {
            data->logged_in = true;
            ask_for_history(data);
            return;
        }
        pair_t *p = calloc(1, sizeof(pair_t));
        p->first = data;
        p->second = NULL;
        login_callback_failure(p);
        return;
    }

    char *msg = NULL;
    asprintf(&msg, "Recieved status code %d (%s)", evt->status, status_str(evt->status));
    send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
            MESSAGE_CLIENT_INTERNAL, NULL, msg, get_tstamp()
    )));
}
