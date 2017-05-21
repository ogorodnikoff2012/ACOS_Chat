//
// Created by xenon on 20.05.17.
//

#ifndef XENON_SMASH_STRING_BUFFER_H
#define XENON_SMASH_STRING_BUFFER_H

#include "ts_vector/ts_vector.h"

typedef ts_vector_t string_buffer_t;

void string_buffer_init(string_buffer_t *buf);
void string_buffer_destroy(string_buffer_t *buf);

void string_buffer_append(string_buffer_t *buf, const char *str);
void string_buffer_n_append(string_buffer_t *buf, int n, const char *str);

char *string_buffer_extract(string_buffer_t *buf);

#endif //XENON_SMASH_STRING_BUFFER_H
