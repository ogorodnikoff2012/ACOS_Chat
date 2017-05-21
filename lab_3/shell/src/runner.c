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
#include <string.h>
#include <chdir.h>
#include <readline/history.h>

static ts_vector_t bg_jobs;

int last_signal = -1;

void bg_jobs_stack_init() {
    ts_vector_init(&bg_jobs, sizeof(int));
    atexit(bg_jobs_stack_destroy);
}

void bg_jobs_stack_destroy() {
    ts_vector_destroy(&bg_jobs);
}

static void bg(int pgid) {
    kill(pgid, SIGTSTP);
    ts_vector_push_back(&bg_jobs, &pgid);
}

static void fg(int pgid) {
    kill(pgid, SIGCONT);
    int status = 0;
    while (true) {
        int rc = waitpid(-pgid, &status, 0);
        if (rc == -1) {
            if (errno == EINTR) {
                kill(pgid, last_signal);
                bg(pgid);
                break;
            } else if (errno == ECHILD) {
                int *jobs = bg_jobs.data;
                for (size_t i = 0; i < bg_jobs.size; ++i) {
                    if (jobs[i] == pgid) {
                        jobs[i] = -jobs[i];
                    }
                }
                break;
            }
        }
    }
}

#define MAX_HIST_PRINT_LENGTH 1000

static bool process_internal_command(command_t *cmd) {
    char **argv = cmd->args.data;
    size_t argc = cmd->args.size - 1;

    if (argc == 0) {
        return false;
    }
    if (strcmp(argv[0], "history") == 0) {
        HIST_ENTRY **list = history_list();
        int length = history_length;
        for (int i = 0; i < MAX_HIST_PRINT_LENGTH; ++i) {
            int idx = length - MAX_HIST_PRINT_LENGTH + i;
            if (idx > 0) {
                printf("%-8d%s\n", idx + 1, list[idx]->line);
            }
        }
        return true;
    }
    return false;
}

static bool process_special_command(command_t *cmd) {
    char **argv = cmd->args.data;
    size_t argc = cmd->args.size;
    if (argc == 0) {
        return false;
    }
    if (strcmp(argv[0], "cd") == 0) {
        if (argc < 2) {
            fprintf(stderr, "cd: missing argument\n");
            return true;
        }
        cd(argv[1]);
        return true;
    } else if (strcmp(argv[0], "pushd") == 0) {
        if (argc < 2) {
            fprintf(stderr, "pushd: missing argument\n");
            return true;
        }
        pushd(argv[1]);
        return true;
    } else if (strcmp(argv[0], "popd") == 0) {
        popd();
        return true;
    } else if (strcmp(argv[0], "set") == 0) {
        if (argc < 3) {
            fprintf(stderr, "set: missing arguments\n");
            return true;
        }
        setenv(argv[1], argv[2], 1);
        return true;
    } else if (strcmp(argv[0], "fg") == 0) {
        int pgid = -1;
        while (pgid < 0) {
            if (bg_jobs.size == 0) {
                printf("No background jobs\n");
                return true;
            }
            int *jobs = bg_jobs.data;
            pgid = jobs[bg_jobs.size-- - 1];
        }
        fg(pgid);
        return true;
    } else if (strcmp(argv[0], "bg") == 0) {
        printf("Not implemented yet");
        return true;
    } else if (strcmp(argv[0], "jobs") == 0) {
        printf("Current background jobs:\n");
        int *jobs = bg_jobs.data;
        for (size_t i = 0; i < bg_jobs.size; ++i) {
            if (jobs[i] > 0) {
                printf("[%lu] %d\n", bg_jobs.size - i, jobs[i]);
            }
        }
        return true;
    } else if (strcmp(argv[0], "exit") == 0) {
        exit(0);
        return true; // Yes, I know that this line is unreachable
    }
    return false;
}

void run_command_group(command_group_t *grp, struct termios *info) {
/*    if (grp->daemon || grp->background) {
        printf("Sorry, background jobs and daemons aren't supported yet :-(\n");
        return;
    }*/
    size_t cnt = grp->commands.size;

    if (cnt == 1) { // It's time to special commands
        command_t *cmd = *((command_t **)(grp->commands.data));
        if (process_special_command(cmd)) {
            return;
        }
    }

    int pgid = -1;

    int last_fd = STDIN_FILENO;
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

            if (process_internal_command(cmd)) {
                exit(0);
            }
            execvp(argv[0], argv);
            error(2, errno, "exec() failed");
        } else {
            if (pgid < 0) {
                pgid = result;
            }
            int rc = setpgid(result, pgid);
            if (rc < 0) {
                error(3, errno, "setpgid() failed");
            }
            if (last_fd != STDIN_FILENO) {
                close(last_fd);
            }
            if (out_fd != STDOUT_FILENO) {
                close(out_fd);
            }
            last_fd = in_fd;
        }
    }

    if (last_fd != STDIN_FILENO) {
        close(last_fd);
    }

    if (grp->daemon || grp->background) {
        bg(pgid);
    } else {
        fg(pgid);
    }
}
