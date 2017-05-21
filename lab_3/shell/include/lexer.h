//
// Created by xenon on 20.05.17.
//

#ifndef XENON_SMASH_LEXER_H
#define XENON_SMASH_LEXER_H

#include <stdbool.h>

typedef enum {
#define ENUM_OR_STR(x) x,
#include "token_types.h"
#undef ENUM_OR_STR
} token_type_t;

extern const char *TOKEN_TYPE[];

typedef struct {
    token_type_t type;
    bool closed, escaped, separated_from_previous;
    char *text;
} token_t;

token_t *next_token(const char **str, token_t *previous_token);
void free_token(token_t *t);

#endif // XENON_SMASH_LEXER_H
