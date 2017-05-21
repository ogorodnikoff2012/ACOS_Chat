//
// Created by xenon on 21.05.17.
//

#include <runner.h>
#include <stdio.h>
#include <command.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <wait.h>
#include <fcntl.h>

void run_command_group(command_group_t *grp, struct termios *info) {
    if (grp->daemon || grp->background) {
        printf("Sorry, background jobs and daemons aren't supported yet :-(\n");
        return;
    }
    size_t cnt = grp->commands.size;

    int last_fd = STDIN_FILENO;
    int *children = calloc(cnt, sizeof(int));
    for (int i = 0; i < cnt; ++i) {
        command_t *cmd = ((command_t **)(grp->commands.data))[i];
        char *null = NULL;
        ts_vector_push_back(&cmd->args, &null);
        char **argv = cmd->args.data;

        int pipe_fds[2];
        int in_fd, out_fd;

        if (i < cnt - 1) {
            if (pipe(pipe_fds) < 0) {
                error(2, errno, "failed while piping");
            }
        } else {
            pipe_fds[1] = STDOUT_FILENO;
        }

        in_fd = pipe_fds[0];
        out_fd = pipe_fds[1];

        int result = fork();
        if (result < 0) {
            error(2, errno, "failed while forking");
        }
        if (result == 0) {
            if (cmd->fin != NULL) {
                FILE *f = fopen(cmd->fin, "r");
                if (f == NULL) {
                    error(2, errno, "Cannot open file %s", cmd->fin);
                }
                int fd = fileno(f);
                dup2(fd, last_fd);
                close(fd);
            }
            if (cmd->fout != NULL) {
                FILE *f = fopen(cmd->fout, cmd->fout_append ? "a" : "w");
                if (f == NULL) {
                    error(2, errno, "Cannot open file %s", cmd->fin);
                }
                int fd = fileno(f);
                dup2(fd, out_fd);
                close(fd);
            }
            dup2(last_fd, STDIN_FILENO);
            dup2(out_fd, STDOUT_FILENO);

/*            if (last_fd != STDIN_FILENO) {
                close(last_fd);
            }
            if (pipe_fds[0] != STDOUT_FILENO) {
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }*/

            execvp(argv[0], argv);
            error(2, errno, "No such program!");
        } else {
            if (last_fd != STDIN_FILENO) {
                close(last_fd);
            }
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
            last_fd = in_fd;

            children[i] = result;
        }
    }

    if (last_fd != STDIN_FILENO) {
        close(last_fd);
    }

    for (int i = 0; i < cnt; ++i) {
        int status = 0, w;
        while ((w = waitpid(children[i], &status, 0)) < 0) {
            if (errno == ECHILD) {
                break;
            }
        }
    }
    free(children);
}
