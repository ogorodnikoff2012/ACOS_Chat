//
// Created by xenon on 20.05.17.
//

#ifndef XENON_SMASH_COMMAND_H
#define XENON_SMASH_COMMAND_H

#include <stdbool.h>
#include <ts_vector/ts_vector.h>

typedef struct {
    ts_vector_t args;
    char *fin, *fout;
    bool fout_append;
} command_t;

typedef struct {
    bool background, daemon;
    ts_vector_t commands;
} command_group_t;

void destroy_command_group(command_group_t *grp);
void destroy_command(command_t *cmd);
void free_command_group(command_group_t *grp);
void free_command(command_t *cmd);

#endif // XENON_SMASH_COMMAND_H
