//
// Created by xenon on 09.05.17.
//

#ifndef XENON_CHAT_SERVER_SERVER_MESSAGE_H
#define XENON_CHAT_SERVER_SERVER_MESSAGE_H

#include <stdint.h>
#include "../pascal_string.h"
#include "../ts_vector/ts_vector.h"

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
} server_message_t;

void delete_server_message(server_message_t *msg);
server_message_t *new_server_message(char msg_type, ts_vector_t *tokens);
void *pack_message(server_message_t *msg);

#endif // XENON_CHAT_SERVER_SERVER_MESSAGE_H
