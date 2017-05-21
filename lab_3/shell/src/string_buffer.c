//
// Created by xenon on 20.05.17.
//

#include <string_buffer.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>

void string_buffer_init(string_buffer_t *buf) {
    ts_vector_init(buf, sizeof(char));
    char c = '\0';
    ts_vector_push_back(buf, &c);
}

void string_buffer_destroy(string_buffer_t *buf) {
    ts_vector_destroy(buf);
}

void string_buffer_append(string_buffer_t *buf, const char *str) {
    --buf->size;
    while (*str != '\0') {
        ts_vector_push_back(buf, (void *) (str++));
    }
    char c = '\0';
    ts_vector_push_back(buf, &c);
}

void string_buffer_n_append(string_buffer_t *buf, int n, const char *str) {
    if (str == NULL) {
        return;
    }
    --buf->size;
    while (*str != '\0' && n > 0) {
        ts_vector_push_back(buf, (void *) (str++));
        --n;
    }
    char c = '\0';
    ts_vector_push_back(buf, &c);
}

char *string_buffer_extract(string_buffer_t *buf) {
    char *str = buf->data;
    buf->data = NULL;
    ts_vector_destroy(buf);
    ts_vector_init(buf, sizeof(char));
    char c = '\0';
    ts_vector_push_back(buf, &c);
    return str;
}
