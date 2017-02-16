#include <parseargs.h>
#include <string.h>

void parse_args(int argc, char* argv[], arg_arr_t arg_arr) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            int j = 1;
            while (argv[i][j]) {
                arg_arr[argv[i][j++]] = true;
            }
        }
    }
}
