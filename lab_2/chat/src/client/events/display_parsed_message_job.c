//
// Created by xenon on 13.05.17.
//

#include <client/events/display_parsed_message_job.h>
#include <client/parsed_message.h>
#include <client/gui.h>
#include <stdlib.h>

void display_parsed_message_job_handler(event_t *ptr, void *dptr) {
    display_parsed_message_job_t *evt = (display_parsed_message_job_t *) ptr;
    gui_data_t *data = (gui_data_t *) dptr;
    gui_print_message(data, evt->msg);
    data->redraw_messages = true;
}

void display_parsed_message_job_deleter(event_t *ptr) {
    display_parsed_message_job_t *evt = (display_parsed_message_job_t *) ptr;
    delete_parsed_message(evt->msg);
    free(evt);
}

display_parsed_message_job_t *new_display_parsed_message_job(parsed_message_t *msg) {
    display_parsed_message_job_t *evt = calloc(1, sizeof(display_parsed_message_job_t));
    evt->e_hdr.type = DISPLAY_PARSED_MESSAGE_JOB_TYPE;
    evt->msg = msg;
    return evt;
}

