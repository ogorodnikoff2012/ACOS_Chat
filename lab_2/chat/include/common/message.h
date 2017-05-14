//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_COMMON_MESSAGE_H
#define XENON_CHAT_COMMON_MESSAGE_H

#include <stdint.h>
#include "pascal_string.h"
#include "ts_vector/ts_vector.h"

typedef enum {
    DATA_C_STR,
    DATA_P_STR,
    DATA_INT32,
    DATA_INT64,
    DATA_NONE
} data_type;

typedef struct {
    union {
        char *c_str;
        uint32_t i32;
        uint64_t i64;
        pascal_string_t *p_str;
    } data;
    data_type type;
} message_token_t;

typedef struct {
    char msg_type;
    ts_vector_t *tokens;
} message_t;

void delete_message(message_t *msg);
message_t *new_message(char msg_type, ts_vector_t *tokens);
void *pack_message(message_t *msg);
message_t *unpack_message_2(char *header, char *body);
message_t *unpack_message_1(char *data);
void send_message(message_t *msg, int sockid);
void send_status_code(int sockid, int status);

#endif // XENON_CHAT_COMMON_MESSAGE_H
