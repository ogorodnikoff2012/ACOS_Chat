//
// Created by xenon on 13.05.17.
//

#include <client/events/send_message_to_server_job.h>
#include <client/defines.h>
#include <stdlib.h>
#include <client/controller.h>
#include <client/parsed_message.h>
#include <string.h>
#include <client/misc.h>
#include <client/events/display_parsed_message_job.h>
#include <common/message.h>

send_message_to_server_job_t *new_send_message_to_server_job(char *text) {
    send_message_to_server_job_t *job = calloc(1, sizeof(send_message_to_server_job_t));
    job->e_hdr.type = SEND_MESSAGE_TO_SERVER_JOB_TYPE;
    job->text = text;
    return job;
}

void send_message_to_server_job_handler(event_t *ptr, void *dptr) {
    send_message_to_server_job_t *job = (send_message_to_server_job_t *) ptr;
    client_controller_data_t *data = dptr;

    if (data->sockid == -1 || !data->logged_in) {
        parsed_message_t *msg = new_parsed_message(MESSAGE_CLIENT_INTERNAL, NULL,
                                                   strdup("Cannot send message: no connection"), get_tstamp());
        send_event(data->gui_event_loop, (event_t *) new_display_parsed_message_job(msg));
        free(job->text);
        return;
    }

    message_token_t token;
    token.type = DATA_C_STR;
    token.data.c_str = job->text;
    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));
    ts_vector_push_back(tokens, &token);

    message_t *msg = new_message(MESSAGE_CLIENT_REGULAR, tokens);
    send_message(msg, data->sockid);
    delete_message(msg);
}

void send_message_to_server_job_deleter(event_t *ptr) {
    send_message_to_server_job_t *job = (send_message_to_server_job_t *) ptr;
    free(job);
}
