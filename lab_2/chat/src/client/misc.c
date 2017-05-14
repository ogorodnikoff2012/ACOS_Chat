//
// Created by xenon on 13.05.17.
//

#include <client/misc.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <client/defines.h>
#include <ts_vector/ts_vector.h>
#include <common/message.h>
#include <client/controller.h>

uint64_t get_tstamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t tstamp = tv.tv_sec;
    tstamp <<= 32;
    return tstamp | (uint32_t) (tv.tv_usec);
}

struct timeval parse_tstamp(uint64_t tstamp) {
    struct timeval tv;
    tv.tv_sec = tstamp >> 32;
    tv.tv_usec = (uint32_t) (tstamp);
    return tv;
}

char *str_strip(const char *str) {
    char *buf = calloc(strlen(str) + 1, sizeof(char));
    char *buf_it = buf;
    char prev = '\n';
    for (const char *it = str; *it != 0; ++it) {
        bool copy = true;
        if (*it == prev && prev <= ' ') {
            copy = false;
        }
        if (copy) {
            *buf_it++ = *it;
        }
        prev = *it;
    }
    --buf_it;
    while (buf_it > buf && *buf_it == ' ') {
        *buf_it-- = '\0';
    }
    return buf;
}

const char *status_str(int status) {
    switch (status) {
        case MSG_STATUS_OK:
            return "OK";
        case MSG_STATUS_INVALID_TYPE:
            return "INVALID TYPE";
        case MSG_STATUS_UNAUTHORIZED:
            return "UNAUTHORIZED";
        case MSG_STATUS_AUTH_ERROR:
            return "AUTH_ERROR";
        case MSG_STATUS_REG_ERROR:
            return "REG ERROR";
        case MSG_STATUS_ACCESS_ERROR:
            return "ACCESS ERROR";
        case MSG_STATUS_INVALID_MSG:
            return "INVALID MSG";
        default:
            return "UNKNOWN STATUS";
    }
}

void ask_for_history(client_controller_data_t *data) {
    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));
    message_token_t token;

    token.type = DATA_INT32;
    token.data.i32 = MAX_HISTORY_LENGTH;
    ts_vector_push_back(tokens, &token);

    message_t *msg = new_message(MESSAGE_CLIENT_HISTORY, tokens);
    send_message(msg, data->sockid);
    delete_message(msg);
}

void ask_for_userlist(client_controller_data_t *data) {
    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    message_t *msg = new_message(MESSAGE_CLIENT_LIST, tokens);
    send_message(msg, data->sockid);
    delete_message(msg);
}