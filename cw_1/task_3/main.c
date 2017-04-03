#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

int proc_id[] = {-1, -1, -1};

void sig_handler(int sigtype) {
    int sig = sigtype == SIGUSR1 ? SIGTERM : SIGKILL;
    for (int i = 0; i < 3; ++i) {
        kill(proc_id[i], sig);
    }
}

int process_task(int argc, char *argv[]) {
    if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
        fprintf(stderr, "Can't catch signal SIGUSR1\n");
        exit(2);
    }
    if (signal(SIGUSR2, sig_handler) == SIG_ERR) {
        fprintf(stderr, "Can't catch signal SIGUSR2\n");
        exit(2);
    }
    if ((proc_id[0] = fork()) == 0) {
        FILE *f = fopen(argv[1], "a");
        if (f == NULL) {
            error(2, errno, "Cannot open file %s", argv[1]);
        }
        int fd = fileno(f);
        dup2(fd, STDOUT_FILENO);
        if (execve(argv[0], argv, (char **)NULL) == -1) {
            error(2, errno, "execve() pr1 failed");
        }
    } else if (proc_id[0] > 0) {
        int status;
        int res = waitpid(proc_id[0], &status, 0);
        if (res == -1) {
            kill(proc_id[0], SIGKILL);
            error(2, errno, "wait() pr1 failed");
        }

        if (status != 0) {
            return status;
        }

        int fd[2];
        res = pipe(fd);

        if (res == -1) {
            error(2, errno, "pipe() failed");
        }

        if ((proc_id[1] = fork()) == 0) {
            dup2(fd[1], STDOUT_FILENO);
            if (execve(argv[2], argv + 2, (char **)NULL) == -1) {
                error(2, errno, "execve() pr2 failed");
            }
        } else if (proc_id[1] > 0) {
            if ((proc_id[2] = fork()) == 0) {
                dup2(fd[0], STDIN_FILENO);
                if (execve(argv[3], argv + 3, (char **)NULL) == -1) {
                    kill(proc_id[1], SIGKILL);
                    error(2, errno, "execve() pr3 failed");
                }
            } else if (proc_id[2] > 0) {
                int res;
                while ((res = waitpid(-1, &status, 0)) > 0) {  };
                return status; 
            } else {
                kill(proc_id[1], SIGKILL);
                error(2, errno, "fork() pr3 failed");
            }
        } else {
            error(2, errno, "fork() pr2 failed");
        }
    } else {
        error(2, errno, "fork() pr1 failed");
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <pr1> <res> <pr2> <pr3> <nom_of_secs>\n", argv[0]);
        return 1;
    }

    int pid_helper;
    if ((pid_helper = fork()) == 0) {
        return process_task(argc - 1, argv + 1);
    } else if (pid_helper > 0) {
        int secs = atoi(argv[5]);
        if (secs <= 0) {
            secs = 1;
        }
        sleep(secs - 1);
        kill(pid_helper, SIGUSR1);
        sleep(1);
        kill(pid_helper, SIGUSR2);
    }
    kill(pid_helper, SIGTERM);
    return 0;
}
