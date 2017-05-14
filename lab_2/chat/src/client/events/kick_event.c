//
// Created by xenon on 14.05.17.
//

#include <client/events/kick_event.h>
#include <stdlib.h>
#include <client/controller.h>
#include <client/events/display_parsed_message_job.h>
#include <client/defines.h>
#include <string.h>
#include <client/misc.h>
#include <stdio.h>

kick_event_t *new_kick_event(char *str) {
    kick_event_t *evt = calloc(1, sizeof(kick_event_t));
    evt->e_hdr.type = KICK_EVENT_TYPE;
    evt->reason = str;
    return evt;
}

void kick_event_handler(event_t *ptr, void *dptr) {
    client_controller_data_t *data = dptr;
    kick_event_t *evt = (kick_event_t *) ptr;

    char *msg = NULL;
    asprintf(&msg, "You have been kicked, reason: \"%s\"", evt->reason);
    send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
            MESSAGE_CLIENT_INTERNAL, NULL, msg, get_tstamp()
    )));
    send_urgent_event(&data->event_loop, &EXIT_EVT);
}

void kick_event_deleter(event_t *ptr) {
    kick_event_t *evt = (kick_event_t *) ptr;
    free(evt->reason);
    free(evt);
}
