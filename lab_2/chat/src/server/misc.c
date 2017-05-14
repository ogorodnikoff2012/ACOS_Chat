//
// Created by xenon on 09.05.17.
//

#include <server/misc.h>
#include <netinet/in.h>
#include <server/defines.h>
#include <common/logger.h>
#include <sys/time.h>
#include <stdlib.h>

uint64_t get_tstamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t tstamp = tv.tv_sec;
    tstamp <<= 32;
    return tstamp | (uint32_t) (tv.tv_usec);
}

uint64_t unique_id() {
    static uint64_t id = 0;
    return ++id;
}

char *prompt(const char *str) {
    printf("%s: ", str);
    fflush(stdout);
    char *s = NULL;
    size_t n = 0;
    int len = getline(&s, &n, stdin);
    if (len < 0) {
        free(s);
        return NULL;
    }
    return s;
}

int to_range(int min, int val, int max) {
    return val < min ? min : val > max ? max : val;
}
