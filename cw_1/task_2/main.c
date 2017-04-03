#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>

int min(int a, int b) {
    return a > b ? b : a;
}

void print_asterisks(int cnt) {
    while (cnt--) {
        printf("*");
    }
}

void process_line(char *line, size_t len) {
    int sum = 0, count = 0;
    for (int i = 0; i < len; ++i) {
        char c = line[i];
        if (c >= '0' && c <= '9') {
            sum += c - '0';
            ++count;
        } else {
            if (count) {
                print_asterisks(min(count, sum));
                count = 0;
                sum = 0;
            }
            printf("%c", c);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        error(2, errno, "Cannot read file %s", argv[1]);
    }

    while (!feof(f)) {
        char *line = NULL;
        size_t len, n = 0;

        len = getline(&line, &n, f);
        if (len != -1LL) {
            process_line(line, len);
        }
        free(line);
    }

    fclose(f);

    return 0;
}
