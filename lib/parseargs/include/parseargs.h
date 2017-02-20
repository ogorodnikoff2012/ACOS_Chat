#ifndef XENON_PARSEARGS_H
#define XENON_PARSEARGS_H

#include <stdbool.h>

#define ADD_ARG(SYMBOL, VARNAME) bool VARNAME; \
    __attribute__ ((__constructor__)) \
    static void parse_args_init_##VARNAME () { \
        _args[SYMBOL] = &VARNAME; \
        VARNAME = false; \
    }

extern bool *_args[256];

void parse_args(int argc, char *argv[]);

#endif // XENON_PARSEARGS_H

