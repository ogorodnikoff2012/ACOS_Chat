#include <stdio.h> 
#include <sys/types.h>
#define __USE_BSD
#include <dirent.h>

typedef struct dirent dirent_t;

#define FILETYPE(NAME) case NAME: return #NAME; 

const char *filetype(unsigned char d_type) {
    switch (d_type) {
        FILETYPE(DT_BLK);
        FILETYPE(DT_CHR);
        FILETYPE(DT_DIR);
        FILETYPE(DT_LNK);
        FILETYPE(DT_REG);
        FILETYPE(DT_WHT);
        FILETYPE(DT_FIFO);
        FILETYPE(DT_SOCK);
        default:
        FILETYPE(DT_UNKNOWN);
    }
}

void process(const dirent_t *entry) {
    printf("%s\t%s\n", filetype(entry->d_type), entry->d_name);
}

int main() {
    DIR *cwd = opendir("./");
    dirent_t *entry = readdir(cwd);
    while (entry != NULL) {
        process(entry);
        entry = readdir(cwd);
    } 
    closedir(cwd);
    return 0;
}
