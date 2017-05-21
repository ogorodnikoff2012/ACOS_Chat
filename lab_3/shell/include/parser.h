//
// Created by xenon on 20.05.17.
//

#ifndef XENON_SMASH_PARSER_H
#define XENON_SMASH_PARSER_H

#include "lexer.h"
#include <stddef.h>
#include <stdbool.h>
#include "command.h"

ts_vector_t *parse(token_t *tokens, size_t size, char **errmsg);

#endif // XENON_SMASH_PARSER_H
