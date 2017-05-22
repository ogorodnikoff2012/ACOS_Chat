#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <signal.h>
#include <termios.h>
#include <lexer.h>
#include <parser.h>
#include <ts_vector/ts_vector.h>
#include <limits.h>
#include <runner.h>
#include <chdir.h>
#include <main.h>

static char *HISTORY_FILE = NULL;

static void sighandler(int signum, siginfo_t *info, void *ptr) {
    last_signal = signum;
}

static void save_history() {
    write_history(HISTORY_FILE);
}

static void setup_sighandlers() {
//    setsid();

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = sighandler;
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &act, NULL);
//    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);

//    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
//    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
}

static void setup_history() {
    using_history();
    asprintf(&HISTORY_FILE, "%s/.smash_history", getenv("HOME"));

    read_history(HISTORY_FILE);
    atexit(save_history);
}

static void setup_vars() {
    setenv("PS1", "\x1B[32;1m>>>\x1B[0m ", 0);
    setenv("PS2", "\x1B[34;1m...\x1B[0m ", 0);
}

static void setup_terminal(struct termios *shell_tmodes) {
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
        error(1, errno, "tcsetpgrp() failed");
    }
/*    if (setpgid(0, 0) < 0) {
        error(1, errno, "Couldn't put the shell in its own process group");
    }*/
    // tcgetattr(STDIN_FILENO, shell_tmodes);
}

static void setup_dirstack() {
    chdir_init();
    atexit(chdir_destroy);
}

static void init_shell(bool interactive) {
    if (interactive) {
        setup_sighandlers();
        setup_history();
        setup_vars();
        setup_terminal(&shell_tmodes);
    }
    setup_dirstack();
    bg_jobs_stack_init();
}

void repl(bool interactive, FILE *f) {
    bool work = true;

    ts_vector_t vector;
    ts_vector_init(&vector, sizeof(token_t));

    while (work) {
        char *input = NULL;
        if (interactive) {
            input = readline(getenv(vector.size > 0 ? "PS2" : "PS1"));
        } else {
            size_t size = 0;
            long int length = getline(&input, &size, f);
            if (length < 0) {
                if (input != NULL) {
                    free(input);
                }
                input = NULL;
            } else {
                if (length > 0) {
                    input[length - 1] = '\0';
                }
            }
        }
        if (input == NULL) {
            break;
        }

        if (interactive) {
            add_history(input);
        }

        const char *iter = input;
        bool first_token = true;
        while (*iter != 0) {
            token_t *prev_token = vector.size == 0 ? NULL : &((token_t *) (vector.data))[vector.size - 1];
            token_t *token = next_token(&iter, prev_token);
            if (first_token) {
                token->separated_from_previous = true;
                first_token = false;
            }
            if (token != NULL) {
                if (token->type == ETT_COMMENT) {
                    free_token(token);
                    continue;
                }
                if (prev_token != NULL && (!prev_token->closed || prev_token->type == ETT_SPLIT)) {
                    if (prev_token->text != NULL) {
                        free(prev_token->text);
                    }
                    --vector.size;
                }
                ts_vector_push_back(&vector, token);
                free(token);
            }
        }

        if (vector.size != 0) {
            token_t *tokens = vector.data;
            if (tokens[vector.size - 1].closed && tokens[vector.size - 1].type != ETT_SPLIT) {
                char *errmsg;
                ts_vector_t *command_groups = parse(tokens, vector.size, &errmsg);
                if (errmsg != NULL) {
                    printf("%s\n", errmsg);
                    free(errmsg);
                }

                for (size_t i = 0; i < command_groups->size; ++i) {
                    command_group_t *group = ((command_group_t **)(command_groups->data))[i];
                    run_command_group(group, &shell_tmodes);
                    free_command_group(group);
                }

                for (int i = 0; i < vector.size; ++i) {
                    if (tokens[i].text != NULL) {
                        free(tokens[i].text);
                    }
                }
                ts_vector_clear(&vector);
            }
        }

        free(input);
    }

    printf("\n");

    if (vector.size != 0) {
        token_t *tokens = vector.data;
        for (int i = 0; i < vector.size; ++i) {
            if (tokens[i].text != NULL) {
                free(tokens[i].text);
            }
        }
    }
    ts_vector_destroy(&vector);
}

int main(int argc, char *argv[]) {
    char *var_buf = malloc(PATH_MAX + 1);
    realpath(argv[0], var_buf);
    setenv("SHELL", var_buf, 1);
    free(var_buf);

    var_buf = NULL;
    asprintf(&var_buf, "%d", getuid());
    setenv("UID", var_buf, 1);
    free(var_buf);

    var_buf = NULL;
    asprintf(&var_buf, "%d", getpid());
    setenv("PID", var_buf, 1);
    free(var_buf);

    var_buf = NULL;
    asprintf(&var_buf, "%d",argc);
    setenv("__SMASH_INTERNAL_ARGC__", var_buf, 1);
    free(var_buf);

    for (int i = 0; i < argc; ++i) {
        char *var_name = NULL;
        asprintf(&var_name, "__SMASH_INTERNAL_ARG_%d__", i);
        setenv(var_name, argv[i], 1);
        free(var_name);
    }

    if (argc > 1) {
        FILE *f = freopen(argv[1], "r", stdin);
        if (f == NULL) {
            error(1, errno, "Cannot open file %s", argv[1]);
        }
    }

    int interactive = isatty(STDIN_FILENO);
    init_shell(interactive > 0);
    repl(interactive > 0, stdin);
    return 0;
}
