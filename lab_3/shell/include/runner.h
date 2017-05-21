//
// Created by xenon on 21.05.17.
//

#ifndef XENON_SMASH_RUNNER_H
#define XENON_SMASH_RUNNER_H

#include "command.h"
#include <termios.h>

void run_command_group(command_group_t *grp, struct termios *info);

#endif // XENON_SMASH_RUNNER_H
