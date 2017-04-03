/* Compile with -std=c99 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BAD_USAGE { usage(argv[0]); exit(1); }
#define min(a, b) (a) < (b) ? (a) : (b);
#define BUFFER_SIZE 4096

int stat(const char *pathname, struct stat *buf);

void usage(char *progname) {
    fprintf(stderr, "Usage: %s <pos1> <pos2> <length> <filename>\n", progname);
}

int read_bit(FILE *f, int pos) {
    int res = fseek(f, pos >> 3, SEEK_SET);
    if (res == -1) {
        fprintf(stderr, "Cannot fseek()\n");
        return -1;
    }
    
    char c;
    res = fread(&c, 1, 1, f);
    if (res != 1) {
        fprintf(stderr, "Cannot fread()\n");
        return -1;
    }

    return (c & (1 << (pos & 7))) ? 1 : 0;
}

int read_bits(FILE *f, int pos, int count, char *ans) {
    int offset = 0;
    for (int i = 0; i < count; ++i) {
        int bit = read_bit(f, pos + i);
        if (bit == -1) {
            fprintf(stderr, "Cannot read bit #%d\n", pos + i);
            return -1;
        }
        ans[i >> 3] = (ans[i >> 3] & ~(1 << offset)) | (bit << (offset));
        ++offset;
        offset &= 7;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        BAD_USAGE;
    }

    char *ptr;
    int pos1 = strtol(argv[1], &ptr, 10);
    if (*ptr != 0) {
        BAD_USAGE;
    }

    int pos2 = strtol(argv[2], &ptr, 10);
    if (*ptr != 0) {
        BAD_USAGE;
    }

    int length = strtol(argv[3], &ptr, 10);
    if (*ptr != 0) {
        BAD_USAGE;
    }

    FILE *f = fopen(argv[4], "r");
    if (f == NULL) {
        error(2, errno, "Cannot open file %s", argv[4]);
    }
    
    long long answer = 0;

    char first_buf[BUFFER_SIZE], second_buf[BUFFER_SIZE];
    for (int i = 0; i < length; i += BUFFER_SIZE << 3) {
        int part_length = min(length - i, BUFFER_SIZE);
        int res = read_bits(f, pos1 + i, part_length, first_buf);
        if (res == -1) {
            error(2, errno, "Cannot read bits! Halting");
        }
        res = read_bits(f, pos2 + i, part_length, second_buf);
        if (res == -1) {
            error(2, errno, "Cannot read bits! Halting");
        }

        for (int j = 0; j < part_length; ++j) {
            int first_bit = first_buf[j >> 3] & (1 << (j & 7));
            int second_bit = second_buf[j >> 3] & (1 << (j & 7));
            if (first_bit ^ second_bit) {
                ++answer;
            }
        }
    }

    fclose(f);

    printf("%lld\n", answer);
    return 0;
}
