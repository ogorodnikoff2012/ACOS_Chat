//
// Created by xenon on 21.05.17.
//

#ifndef XENON_SMASH_CHDIR_H
#define XENON_SMASH_CHDIR_H

#include <stdbool.h>

bool cd(const char *dir);
bool pushd(const char *dir);
bool popd();

void chdir_init();
void chdir_destroy();

#endif //XENON_SMASH_CHDIR_H
