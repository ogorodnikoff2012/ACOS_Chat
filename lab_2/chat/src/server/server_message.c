//
// Created by xenon on 09.05.17.
//

#include <server/server_message.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>
#include <server/defines.h>
#include <string.h>
#include <logger.h>
#include <netinet/in.h>

void delete_server_message(server_message_t *msg) {
    for (int i = 0; i < msg->tokens->size; ++i) {
        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR:
                free(token.data.c_str);
                break;
            case DATA_INT64:
                break;
            case DATA_INT32:
                break;
            case DATA_P_STR:
                free(token.data.p_str);
                break;
            default:
                break;
        }
    }
    ts_vector_destroy(msg->tokens);
    free(msg->tokens);
    free(msg);
}

void *pack_message(server_message_t *msg) {
    uint32_t size = MSG_HEADER_SIZE;
    for (int i = 0; i < msg->tokens->size; ++i) {
        size += 4;
        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR:
                size += strlen(token.data.c_str);
                break;
            case DATA_INT64:
                size += sizeof(uint64_t);
                break;
            case DATA_INT32:
                size += sizeof(uint32_t);
                break;
            case DATA_P_STR:
                size += token.data.p_str->length;
                break;
            default:
                LOG("Corrupted message");
                break;
        }
    }
    void *buffer = malloc(size);
    void *it = buffer;

    *(char *)it = msg->msg_type;
    it += sizeof(char);
    *(uint32_t *)it = htonl(size - MSG_HEADER_SIZE);
    it += sizeof(uint32_t);

    for (int i = 0; i < msg->tokens->size; ++i) {
        uint32_t *len_ptr = (uint32_t *) it;
        it += sizeof(uint32_t);

        message_token_t token = ((message_token_t *) (msg->tokens->data))[i];
        switch (token.type) {
            case DATA_C_STR: {
                *len_ptr = strlen(token.data.c_str);
                memcpy(it, token.data.c_str, *len_ptr);
            }
                break;
            case DATA_INT64:
                *len_ptr = sizeof(uint64_t);
                *((uint64_t *) it) = htobe64(token.data.i64);
                break;
            case DATA_INT32:
                *len_ptr = sizeof(uint32_t);
                *((uint32_t *) it) = htonl(token.data.i32);
                break;
            case DATA_P_STR:
                *len_ptr = token.data.p_str->length;
                memcpy(it, token.data.p_str->data, *len_ptr);
                break;
            default:
                *len_ptr = 0;
                break;
        }
        it += *len_ptr;
        *len_ptr = htonl(*len_ptr);
    }
    return buffer;
}

server_message_t *new_server_message(char msg_type, ts_vector_t *tokens) {
    server_message_t *msg = calloc(1, sizeof(server_message_t));
    msg->msg_type = msg_type;
    msg->tokens = tokens;
    return msg;
}
