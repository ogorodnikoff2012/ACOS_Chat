#include <unistd.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <parseargs/parseargs.h>
#include <string.h>

#define RWX_STRING "rwxrwxrwx"

typedef struct dirent dirent_t;
typedef struct stat stat_t;

ADD_ARG('l', show_long_info)
ADD_ARG('a', show_all)

char filetype_suffix(__mode_t st_mode) {
    switch(st_mode) {
        case __S_IFDIR: return '/';
        case __S_IFIFO: return '|';
        case __S_IFLNK: return '@';
        default: return ' ';
    }
}

char filetype_prefix(__mode_t st_mode) {
    switch (st_mode) {
        case __S_IFDIR: return 'd';
        default: return '-';
    }
}

const char *rwx(unsigned long mode) {
    static char str[10];
    str[9] = 0;
    for (int i = 0, j = 1 << 8; i < 9; ++i, j >>= 1) {
        str[i] = (mode & j) ? RWX_STRING[i] : '-';
    }
    return str;
}

void process_direntry(DIR *dir, const dirent_t *entry) {
    const char *fname = entry->d_name;
    
    if (!show_all && fname[0] == '.') { return; }

    stat_t s;
    fstatat(dirfd(dir), fname, &s, 0);
    if (show_long_info) {
        printf("%c%s ", 
                filetype_prefix(s.st_mode & __S_IFMT),
                rwx(s.st_mode));
    }
    printf("%s%c\n", fname, filetype_suffix(s.st_mode & __S_IFMT));
}

void process_dir(const char *dirname) {
    DIR *dir = opendir(dirname);
    dirent_t *entry = readdir(dir);
    while (entry != NULL) {
        process_direntry(dir, entry);
        entry = readdir(dir);
    } 
    closedir(dir);
}

int main(int argc, char *argv[]) {
    parse_args(argc, argv);

#ifdef DEBUG
    printf("Show long info: %d; show all: %d\n", show_long_info, show_all);
#endif

    int processed_dirs_cnt = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            printf("%s:\n", argv[i]);
            process_dir(argv[i]);
            ++processed_dirs_cnt;
        }
    }

    if (!processed_dirs_cnt) {
        process_dir("./");
    }

    return 0;
}
