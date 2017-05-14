//
// Created by xenon on 14.05.17.
//

#include <client/events/hangup_event.h>
#include <client/defines.h>
#include <client/controller.h>
#include <client/events/display_parsed_message_job.h>
#include <string.h>
#include <client/misc.h>

static hangup_event_t HANGUP_EVT;

hangup_event_t *new_hangup_event() {
    HANGUP_EVT.e_hdr.type = HANGUP_EVENT_TYPE;
    return &HANGUP_EVT;
}
void hangup_event_handler(event_t *ptr, void *dptr) {
    client_controller_data_t *data = dptr;

    send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(new_parsed_message(
        MESSAGE_CLIENT_INTERNAL, NULL, strdup("Server has been disconnected, please restart client."), get_tstamp()
    )));
    send_urgent_event(&data->event_loop, &EXIT_EVT);
}

void hangup_event_deleter(event_t *ptr) {
    // Do nothing
}
