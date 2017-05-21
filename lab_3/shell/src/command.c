//
// Created by xenon on 21.05.17.
//


#include <command.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>

void destroy_command_group(command_group_t *grp) {
    for (int i = 0; i < grp->commands.size; ++i) {
        free_command(((command_t **)(grp->commands.data))[i]);
    }
    ts_vector_destroy(&grp->commands);
}

void free_command_group(command_group_t *grp) {
    destroy_command_group(grp);
    free(grp);
}

void destroy_command(command_t *cmd) {
    for (int i = 0; i < cmd->args.size; ++i) {
        free(((char **)(cmd->args.data))[i]);
    }
    if (cmd->fin != NULL) {
        free(cmd->fin);
    }
    if (cmd->fout != NULL) {
        free(cmd->fout);
    }
    ts_vector_destroy(&cmd->args);
}

void free_command(command_t *cmd) {
    destroy_command(cmd);
    free(cmd);
}
