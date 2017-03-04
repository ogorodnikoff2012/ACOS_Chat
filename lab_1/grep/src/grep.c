#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define ANSI_RED_COLOR "\e[31;1m"
#define ANSI_GREEN_COLOR "\e[32;1m"
#define ANSI_RESET_COLOR "\e[0m"

typedef bool (* match_t)(char, char);

typedef struct {
    const char *str;
    int *pi_func;
    int length;
    match_t match;
} pattern_t;

pattern_t *build_pattern(const char *str, match_t match) {
    pattern_t *res = (pattern_t *) calloc(sizeof(pattern_t), 1);
    int len = strlen(str);
    res->length = len;
    char *s_copy = (char *) calloc(sizeof(char), len + 1);
    strncpy(s_copy, str, len);
    res->str = s_copy;
    res->pi_func = (int *) calloc(sizeof(int), len);
    res->match = match;

    for (int i = 1; i < len; ++i) {
        res->pi_func[i] = res->pi_func[i - 1];
        while (res->pi_func[i] > 0 && !match(str[i], str[res->pi_func[i]])) {
            res->pi_func[i] = res->pi_func[res->pi_func[i] - 1];
        }
        if (match(str[i], str[res->pi_func[i]])) {
            ++res->pi_func[i];
        }
    }
    return res;
}

int match_pattern(const pattern_t *pattern, const char *str) {
    int best_suffix_len = 0;
    int s_len = strlen(str);
    for (int i = 0; i < s_len; ++i) {
        while (best_suffix_len > 0 && !pattern->match(str[i], pattern->str[best_suffix_len])) {
            best_suffix_len = pattern->pi_func[best_suffix_len - 1];
        }
        if (pattern->match(str[i], pattern->str[best_suffix_len])) {
            ++best_suffix_len;
        }
        if (best_suffix_len == pattern->length) {
            return i - best_suffix_len + 1;
        }
    }
    return -1;
}

void free_pattern(pattern_t *pattern) {
    free((void *) pattern->str);
    free((void *) pattern->pi_func);
    free((void *) pattern);
}

bool match_strict(char a, char b) {
    return a == b;
}

bool match_ignore_case(char a, char b) {
    return tolower(a) == tolower(b);
}

match_t select_match_func(bool ignore_case) {
    if (ignore_case) {
        return match_ignore_case;
    } else {
        return match_strict;
    }
}

void process_file(FILE *f, pattern_t *pattern, bool invert_match) {
    char *line = NULL;
    size_t len = 0, read;
    while ((read = getline(&line, &len, f)) != -1) {
        int pos = match_pattern(pattern, line);
        if ((pos >= 0) ^ invert_match) {
            if (pos >= 0) {
                printf("%.*s" ANSI_RED_COLOR "%.*s" ANSI_RESET_COLOR "%s", pos, line, pattern->length, line + pos, line + pos + pattern->length);
            } else {
                fputs(line, stdout);
            }
        }   
    }
    free(line);
}

int main(int argc, char *argv[]) {
    int opt;
    bool ignore_case = false, invert_match = false;
    while ((opt = getopt(argc, argv, "iv")) != -1) {
        switch (opt) {
            case 'i':
                ignore_case = true;
                break;
            case 'v':
                invert_match = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-i] [-v] FILE1 [FILE2 ...]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    
    int file_count = argc - optind - 1;

    if (file_count <= 0) {
        fprintf(stderr, "Error: no input file\n");
        return EXIT_FAILURE;
    }
    
    pattern_t *pattern = build_pattern(argv[optind], select_match_func(ignore_case));

    for (int i = optind + 1; i < argc; ++i) {
        const char *fname = argv[i];
        FILE *f = NULL;
        if (strcmp(fname, "-") == 0) {
            f = stdin;
            fname = "stdin";
        } else {
            f = fopen(fname, "r");
        }
        if (file_count > 1) {
            printf(ANSI_GREEN_COLOR "%s" ANSI_RESET_COLOR ":\n", fname);
        }
        if (f == NULL) {
            fprintf(stderr, "Error: cannot open file %s: %s\n", fname, strerror(errno));
            continue;
        }
        process_file(f, pattern, invert_match);
        fclose(f);
    }

    free_pattern(pattern);

    return EXIT_SUCCESS;
}