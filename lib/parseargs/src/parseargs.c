#include <parseargs.h>
#include <string.h>

bool *_args[256];

void parse_args(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            int j = 1;
            while (argv[i][j]) {
                bool *ptr = _args[argv[i][j++]];
                if (ptr != NULL) {
                    *ptr = true;
                }
            }
        }
    }
}
