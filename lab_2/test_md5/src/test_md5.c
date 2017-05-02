#include <xenon_md5/xenon_md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char *input = NULL;
    size_t n = 0;
    getline(&input, &n, stdin);
    printf("%s\n", md5hex(input));
    free(input);
    return 0;
}
