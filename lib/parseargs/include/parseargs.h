#ifndef XENON_PARSEARGS_H
#define XENON_PARSEARGS_H

#include <stdbool.h>

#define BEGIN_ARG_ENUM typedef enum {
#define ADD_ARG(SYMBOL, VARNAME) VARNAME = SYMBOL,
#define END_ARG_ENUM } arg_off_t;

typedef bool arg_arr_t[256];

void parse_args(int argc, char* argv[], arg_arr_t arg_arr);

#endif // XENON_PARSEARGS_H

