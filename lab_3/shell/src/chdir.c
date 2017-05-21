//
// Created by xenon on 21.05.17.
//

#include <chdir.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ts_vector/ts_vector.h>
#include <stdlib.h>
#include <limits.h>

static bool is_dir(const char *dir) {
    struct stat sb;
    return stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode);
}

static ts_vector_t dir_stack;

static char *popd_internal() {
    char **arr = dir_stack.data;
    return arr[dir_stack.size-- - 1];
}

static void pushd_internal(const char *dir) {
    char *s = strdup(dir);
    ts_vector_push_back(&dir_stack, &s);
}

static void dup_internal() {
    char **arr = dir_stack.data;
    char *s = strdup(arr[dir_stack.size - 1]);
    ts_vector_push_back(&dir_stack, &s);
}

bool cd(const char *dir) {
    int result = chdir(dir);
    if (result < 0) {
        fprintf(stderr, "Cannot change directory: %s\n", strerror(errno));
        return false;
    }
    free(popd_internal());
    pushd_internal(dir);
    return true;
}

bool pushd(const char *dir) {
    dup_internal();
    bool result = cd(dir);
    if (!result) {
        free(popd_internal());
    }
    return result;
}

bool popd() {
    if (dir_stack.size < 2) {
        fprintf(stderr, "Dir stack is empty\n");
        return false;
    }
    char *cur_dir = popd_internal();
    dup_internal();
    char *old_dir = popd_internal();

    bool result = cd(old_dir);
    free(old_dir);

    if (!result) {
        pushd_internal(cur_dir);
    }
    free(cur_dir);
    return result;
}

void chdir_init() {
    ts_vector_init(&dir_stack, sizeof(char *));
    char *buf = malloc(PATH_MAX + 1);
    getcwd(buf, PATH_MAX + 1);
    pushd_internal(buf);
    free(buf);
}

void chdir_destroy() {
    char **arr = dir_stack.data;
    for (int i = 0; i < dir_stack.size; ++i) {
        free(arr[i]);
    }
    ts_vector_destroy(&dir_stack);
}
