//
// Created by xenon on 09.05.17.
//

#include <common/message.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>
#include <server/defines.h>
#include <string.h>
#include <common/logger.h>
#include <netinet/in.h>
#include <error.h>
#include <errno.h>

void delete_message(message_t *msg) {
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

void *pack_message(message_t *msg) {
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

message_t *new_message(char msg_type, ts_vector_t *tokens) {
    message_t *msg = calloc(1, sizeof(message_t));
    msg->msg_type = msg_type;
    msg->tokens = tokens;
    return msg;
}

message_t *unpack_message_2(char *header, char *body) {
    int msglen = ntohl(*(int *)(header + 1));

    ts_vector_t *tokens = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(tokens, sizeof(message_token_t));

    char *iter = body, *end = body + msglen;

    while (iter < end) {
        pascal_string_t *str = (pascal_string_t *) iter;
        str->length = ntohl(str->length);
        char *next = iter + sizeof(uint32_t) + str->length;
        if (next > end) {
            LOG("Message corrupted");
            for (int i = 0; i < tokens->size; ++i) {
                free(((message_token_t *) tokens->data)[i].data.p_str);
            }
            ts_vector_destroy(tokens);
            free(tokens);
            return NULL;
        }
        message_token_t token;
        token.type = DATA_P_STR;
        token.data.p_str = pstrdup(str);
        ts_vector_push_back(tokens, &token);
        iter = next;
    }

    return new_message(header[0], tokens);
}

message_t *unpack_message_1(char *data) {
    return unpack_message_2(data, data + MSG_HEADER_SIZE);
}

void send_message(message_t *msg, int sockid) {
    void *package = pack_message(msg);

    int pack_len = ntohl(*(int *)(package + 1)) + MSG_HEADER_SIZE;
    int count = send(sockid, package, pack_len, MSG_NOSIGNAL);
    if (count == -1) {
        LOG("Message sending failed, error: %s", strerror(errno));
    }
    free(package);
}

void send_status_code(int sockid, int status) {
    char buffer[MSG_HEADER_SIZE + 2 * sizeof(uint32_t)];
    buffer[0] = 's';
    *(int *)(buffer + 1) = htonl(2 * sizeof(uint32_t));
    *(int *)(buffer + 5) = htonl(sizeof(uint32_t)); /* Yes, I know that magic constants are awful  */
    *(int *)(buffer + 9) = htonl(status);

    int stat = send(sockid, buffer, MSG_HEADER_SIZE + 2 * sizeof(uint32_t), 0);
    LOG("Sent status code, result = %d", stat);
}
