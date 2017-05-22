//
// Created by xenon on 21.05.17.
//

#ifndef XENON_SMASH_RUNNER_H
#define XENON_SMASH_RUNNER_H

#include "command.h"
#include <termios.h>

extern int last_signal, fg_pgid;
extern struct termios shell_tmodes;

void run_command_group(command_group_t *grp, struct termios *info);
void bg_jobs_stack_init();
void bg_jobs_stack_destroy();

#endif // XENON_SMASH_RUNNER_H
