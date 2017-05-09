//
// Created by xenon on 09.05.17.
//

#include <server/events/send_message_job.h>
#include <server/worker.h>
#include <stdlib.h>
#include <server/server_message.h>
#include <netinet/in.h>
#include <logger.h>
#include <server/misc.h>

void send_message_job_handler(event_t *ptr, void *dptr) {
    send_message_job_t *job = (send_message_job_t *) ptr;
    worker_data_t *data = dptr;

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    message_token_t buf_token;

    buf_token.type = DATA_INT64;
    buf_token.data.i64 = pack_tstamp(job->tstamp);

    ts_vector_push_back(tokens, &buf_token);
    if (job->type == MESSAGE_SERVER_REGULAR) {
        buf_token.type = DATA_C_STR;
        buf_token.data.c_str = job->login;
        ts_vector_push_back(tokens, &buf_token);
    }
    buf_token.type = DATA_C_STR;
    buf_token.data.c_str = job->msg;
    ts_vector_push_back(tokens, &buf_token);

    server_message_t *msg = new_server_message(job->type, tokens);
    void *package = pack_message(msg);
    delete_server_message(msg);

    int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
    int count = send(job->sid, package, pack_len, 0);
    if (count == -1) {
        LOG("Message sending failed");
    }
    free(package);
}

send_message_job_t *new_send_message_job(int sid, uint64_t tstamp, char type, char *login, char *msg) {
    send_message_job_t *job = calloc(1, sizeof(send_message_job_t));
    job->e_hdr.type = SEND_MESSAGE_JOB_TYPE;
    job->sid = sid;
    job->tstamp = tstamp;
    job->login = login;
    job->msg = msg;
    job->type = type;

    return job;
}
