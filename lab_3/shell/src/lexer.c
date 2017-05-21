//
// Created by xenon on 20.05.17.
//

#include <lexer.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ENUM_OR_STR(x) #x ,
const char *TOKEN_TYPE[] = {
#include <token_types.h>
};
#undef ENUM_OR_STR

void free_token(token_t *t) {
    if (t != NULL) {
        if (t->text != NULL) {
            free(t->text);
        }
        free(t);
    }
}

static bool is_special(char c) {
#define SPECIAL_CHARACTERS "<>;|\\&\'\""
    for (size_t i = 0, n = strlen(SPECIAL_CHARACTERS); i < n; ++i) {
        if (c == SPECIAL_CHARACTERS[i]) {
            return true;
        }
    }
    return false;
#undef SPECIAL_CHARACTERS
}

static const char *read_single_quoted(const char *iter, token_t *token) {
    token->closed = true;
    while (*iter > 0 && *iter != '\'') {
        ++iter;
    }
    if (*iter == 0) {
        token->closed = false;
    } else {
        ++iter;
    }
    return iter;
}

static const char *read_double_quoted(const char *iter, token_t *token) {
    token->closed = true;
    bool escaped = false;
    while (*iter > 0 && (escaped || *iter != '\"')) {
        escaped = !escaped && *iter == '\\';
        ++iter;
    }
    if (*iter == 0) {
        token->closed = false;
    } else {
        ++iter;
    }
    token->escaped = escaped;
    return iter;
}

token_t *next_token(const char **str, token_t *prev_token) {
    if (prev_token != NULL && !prev_token->closed) {
        token_t *token = calloc(1, sizeof(token_t));
        token->type = prev_token->type;
        token->separated_from_previous = prev_token->separated_from_previous;
        const char *iter;
        bool prev_escaped = prev_token->escaped;
        switch (prev_token->type) {
            case ETT_SINGLE_QUOTED:
                iter = read_single_quoted(*str, token);
                break;
            case ETT_DOUBLE_QUOTED:
                iter = read_double_quoted(*str, token);
                break;
            default:
                free(token);
                return NULL;
        }

        int new_part_len = (int) (iter - *str);
        char *new_text = NULL;
        if (prev_escaped) {
            size_t len = strlen(prev_token->text);
            prev_token->text[len - 1] = '\0';
            asprintf(&new_text, "%s%.*s", prev_token->text, new_part_len, *str);
        } else {
            asprintf(&new_text, "%s\n%.*s", prev_token->text, new_part_len, *str);
        }

        *str = iter;
        token->text = new_text;
        return token;
    }

    size_t token_length = 0;
    token_t *token = calloc(1, sizeof(token_t));
    token->closed = true;
    token->escaped = false;
    token->separated_from_previous = false;

    while (**str > 0 && **str <= ' ') {
        token->separated_from_previous = true;
        ++(*str);
    }

    const char *token_begin = *str;

    switch (**str) {
        case '\0':
            free(token);
            return NULL;
        case '\n':
        case ';': {
            token_length = 1;
            token->type = ETT_SEPARATOR;
        }
            break;
        case '|': {
            token_length = 1;
            token->type = ETT_PIPE;
        }
            break;
        case '&': {
            token_length = 1;
            token->type = ETT_DAEMON;
        }
            break;
        case '\\': {
            token_length = 1;
            token->type = ETT_SPLIT;
        }
            break;
        case '#': {
            token_length = strlen(token_begin);
            token->type = ETT_COMMENT;
        }
            break;
        case '<':
        case '>': {
            token_length = 1;
            if (token_begin[0] == '>' && token_begin[1] == '>') {
                token_length = 2;
            }
            token->type = ETT_REDIRECT;
        }
            break;
        case '\'': {
            token->type = ETT_SINGLE_QUOTED;
            const char *iter = read_single_quoted(token_begin + 1, token);
            token_length = iter - token_begin;
        }
            break;
        case '\"': {
            token->type = ETT_DOUBLE_QUOTED;
            const char *iter = read_double_quoted(token_begin + 1, token);
            token_length = iter - token_begin;
        }
            break;
        default: {
            const char *iter = token_begin + 1;
            while (*iter > ' ' && !is_special(*iter)) {
                ++iter;
            }
            token_length = iter - token_begin;
        }
    }

    *str += token_length;
    token->text = strndup(token_begin, token_length);
    return token;
}
