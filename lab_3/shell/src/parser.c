//
// Created by xenon on 20.05.17.
//

#include <parser.h>
#include <stdio.h>
#include <string_buffer.h>
#include <stdlib.h>
#include <string.h>
#include <command.h>
#include <lexer.h>

char *parse_plain_token(token_t *token, bool is_first) {
    if (is_first && token->text[0] == '~') {
        string_buffer_t buffer;
        string_buffer_init(&buffer);
        string_buffer_append(&buffer, getenv("HOME"));
        string_buffer_append(&buffer, token->text + 1);
        char *res = string_buffer_extract(&buffer);
        string_buffer_destroy(&buffer);
        return res;
    } else {
        return strdup(token->text);
    }
}

char *parse_single_quoted(token_t *token) {
    char *res = strdup(token->text + 1);
    res[strlen(res) - 1] = '\0';
    return res;
}

int find_subst_end(char *str, int idx) {
    if (str[idx] != '{') {
        return idx + 1;
    }
    int balance = 1;
    ++idx;
    while (str[idx] != '\0' && balance > 0) {
        if (str[idx] == '{') {
            ++balance;
        }
        if (str[idx] == '}') {
            --balance;
        }
        ++idx;
    }
    return balance == 0 ? idx : -1;
}

char *parse_double_quoted_helper(char *str, size_t len) {
    string_buffer_t buffer;
    string_buffer_init(&buffer);

    for (int i = 0; i < len; ++i) {
        if (str[i] == '$') {
            int subst_begin = i + 1;
            int subst_end = find_subst_end(str, subst_begin);
            if (subst_end < 0) {
                string_buffer_destroy(&buffer);
                return NULL;
            }

            int subst_len = subst_end - subst_begin;
            if (subst_len > 2) {
                char *varname = parse_double_quoted_helper(str + subst_begin + 1, (size_t) (subst_len - 2));
                string_buffer_append(&buffer, getenv(varname));
                free(varname);
            } else {
                // Temporarily do nothing
            }
            i = subst_end - 1;
            continue;
        }
        if (str[i] == '\\') {
            if (str[i + 1] == '\\' || str[i + 1] == '"') {
                ++i;
            }
        }
        string_buffer_n_append(&buffer, 1, str + i);
    }

    char *res = string_buffer_extract(&buffer);
    string_buffer_destroy(&buffer);
    return res;

}

char *parse_double_quoted(token_t *token) {
    return parse_double_quoted_helper(token->text + 1, strlen(token->text) - 2);
}

char *parse_string_token(token_t *token, bool is_first) {
    if (token->type == ETT_PLAIN) {
        return parse_plain_token(token, is_first);
    } else if (token->type == ETT_DOUBLE_QUOTED) {
        return parse_double_quoted(token);
    } else if (token->type == ETT_SINGLE_QUOTED) {
        return parse_single_quoted(token);
    }
    return NULL;
}

char *concat_string_tokens(token_t *tokens, size_t limit) {
    string_buffer_t buffer;
    string_buffer_init(&buffer);

    for (int i = 0; i < limit; ++i) {
        char *parsed_str = parse_string_token(&tokens[i], i == 0);
        if (parsed_str == NULL) {
            string_buffer_destroy(&buffer);
            return NULL;
        }
        string_buffer_append(&buffer, parsed_str);
        free(parsed_str);
    }
    char *result = string_buffer_extract(&buffer);
    string_buffer_destroy(&buffer);
    return result;
}

bool is_string_type(token_type_t type) {
    return type == ETT_PLAIN || type == ETT_SINGLE_QUOTED || type == ETT_DOUBLE_QUOTED;
}

int find_end_of_concatenated_tokens(token_t *tokens, int begin, int limit) {
    for (int i = begin + 1; i < limit; ++i) {
        if (tokens[i].separated_from_previous || !is_string_type(tokens[i].type)) {
            return i;
        }
    }
    return limit;
}

#define REDIR_READ 1
#define REDIR_WRITE 2
#define REDIR_APPEND 3

int get_redir_type(char *c) {
    if (c[0] == '<') {
        return REDIR_READ;
    } else if (c[1] == '>') {
        return REDIR_APPEND;
    } else {
        return REDIR_WRITE;
    }
}

command_t *parse_command(token_t *tokens, size_t size, char **errmsg) {
    *errmsg = NULL;
    command_t *cmd = calloc(1, sizeof(command_t));
    ts_vector_init(&cmd->args, sizeof(char *));

    for (int i = 0; i < size; ++i) {
        if (tokens[i].type == ETT_DAEMON) {
            *errmsg = strdup("Bad command group: has ampersand in the middle");
            return cmd;
        }
        int token_group;
        if (tokens[i].type == ETT_REDIRECT) {
            token_group = find_end_of_concatenated_tokens(tokens, i + 1, (int) size);
            char *fname = concat_string_tokens(tokens + i + 1, (size_t) (token_group - i - 1));
            if (fname == NULL) {
                *errmsg = strdup("Bad command group: bad filename");
                return cmd;
            }
            int type = get_redir_type(tokens[i].text);
            char **ptr;
            if (type == REDIR_READ) {
                ptr = &cmd->fin;
            } else {
                ptr = &cmd->fout;
                cmd->fout_append = type == REDIR_APPEND;
            }
            if (*ptr != NULL) {
                free(*ptr);
            }
            *ptr = fname;
        } else {
            token_group = find_end_of_concatenated_tokens(tokens, i, (int) size);
            char *arg = concat_string_tokens(tokens + i, (size_t) (token_group - i));
            if (arg == NULL) {
                *errmsg = strdup("Bad command group: bad string");
                return cmd;
            }
            ts_vector_push_back(&cmd->args, &arg);
        }
        i = token_group - 1;
    }
    return cmd;
}

command_group_t *parse_command_group(token_t *tokens, size_t size, char **errmsg) {
    *errmsg = NULL;
    command_group_t *grp = calloc(1, sizeof(command_group_t));
    ts_vector_init(&grp->commands, sizeof(command_t *));

    if (size > 0 && tokens[size - 1].type == ETT_DAEMON) {
        grp->daemon = true;
        --size;
    }

    if (size == 0) {
        return grp;
    }

    int prev_pipe = -1;
    for (int i = 0; i < size; ++i) {
        if (tokens[i].type == ETT_PIPE) {
            command_t *command = parse_command(tokens + prev_pipe + 1, (size_t) (i - prev_pipe - 1), errmsg);
            ts_vector_push_back(&grp->commands, &command);
            if (*errmsg != NULL) {
                return grp;
            }
            prev_pipe = i;
        }
    }
    if (prev_pipe + 1 == size) {
        *errmsg = NULL;
        asprintf(errmsg, "Bad command group: ends with a pipe");
        return grp;
    }

    command_t *command = parse_command(tokens + prev_pipe + 1, size - prev_pipe - 1, errmsg);
    ts_vector_push_back(&grp->commands, &command);
    return grp;
}

ts_vector_t *parse(token_t *tokens, size_t size, char **errmsg) {
    *errmsg = NULL;
    ts_vector_t *groups = calloc(1, sizeof(ts_vector_t));
    ts_vector_init(groups, sizeof(command_group_t *));

    int prev_sep = -1;
    for (int i = 0; i < size; ++i) {
        if (tokens[i].type == ETT_SEPARATOR) {
            command_group_t *grp = parse_command_group(tokens + (1 + prev_sep), (size_t) (i - prev_sep - 1), errmsg);
            if (*errmsg != NULL) {
                free_command_group(grp);
                return groups;
            }
            ts_vector_push_back(groups, &grp);
            prev_sep = i;
        }
    }
    if (prev_sep + 1 < size) {
        command_group_t *grp = parse_command_group(tokens + (1 + prev_sep), (size_t) (size - prev_sep - 1), errmsg);
        if (*errmsg != NULL) {
            free_command_group(grp);
            return groups;
        }
        ts_vector_push_back(groups, &grp);
    }
    return groups;
}
