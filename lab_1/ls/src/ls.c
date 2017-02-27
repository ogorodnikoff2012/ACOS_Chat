#define __USE_XOPEN 
#define __USE_ATFILE

#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdlib.h>

#define RWX_STRING      "rwxrwxrwx"
#define TIME_FORMAT     "%b %d %Y %H:%M"

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
int dirfd(DIR *dirp);

typedef enum {
    COL_MODE,
    COL_NLINK,
    COL_USER,
    COL_GROUP,
    COL_FSIZE,
    COL_CTIME,
    COL_FNAME,
    COL_TOTAL_COUNT
} col_entry_t;

typedef struct dirent dirent_t;
typedef struct stat stat_t;
typedef struct timespec timespec_t;

bool show_long_info = false;
bool show_all = false;

static inline unsigned long long max_u64(unsigned long long a, unsigned long long b) {
    return a < b ? b : a;
}

static inline unsigned int int_length(unsigned long long n) {
    unsigned int ans = 1;
    while (n >= 10) {
        ++ans;
        n /= 10;
    }
    return ans;
}

char filetype_suffix(mode_t st_mode) {
    switch(st_mode) {
        case S_IFDIR: return '/';
        case S_IFIFO: return '|';
        case S_IFLNK: return '@';
        default: return ' ';
    }
}

char filetype_prefix(mode_t st_mode) {
    switch (st_mode) {
        case S_IFDIR: return 'd';
        case S_IFCHR: return 'c';
        case S_IFBLK: return 'b';
        case S_IFLNK: return 'l';
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

const char *get_change_time(const stat_t *stat) {
    static char str[20];
    strftime(str, 20, TIME_FORMAT, localtime(&stat->st_ctime));
    return str;
}

void prepare_table(DIR *dir, const dirent_t *entry, unsigned int *table) {
    const char *fname = entry->d_name;
    if (!show_all && fname[0] == '.') { 
        return; 
    }

    stat_t s;
    int fd = dirfd(dir);
    if (fd < 0) {
        error(1, errno, "Error while processing %s", fname);
    }
    if (fstatat(fd, fname, &s, AT_SYMLINK_NOFOLLOW)) {
        error(1, errno, "Error while processing %s", fname);
    }
    table[COL_NLINK] = max_u64(table[COL_NLINK], int_length(s.st_nlink));
    table[COL_USER]  = max_u64(table[COL_USER],  strlen(getpwuid(s.st_uid)->pw_name));
    table[COL_GROUP] = max_u64(table[COL_GROUP], strlen(getgrgid(s.st_gid)->gr_name));
    table[COL_FSIZE] = max_u64(table[COL_FSIZE], int_length(s.st_size));
    table[COL_CTIME] = max_u64(table[COL_CTIME], strlen(get_change_time(&s)));
}

void print_symlink(int dirfd, const char *fname) {
    int strsz = 2 * PATH_MAX + 1;
    char *linkname = malloc(strsz);
    if (linkname == NULL) {
        error(1, errno, "Error while reading symlink");
    }
    int r = readlinkat(dirfd, fname, linkname, strsz);
    if (r < 0) {
        error(1, errno, "Error while reading symlink");
    }
   
    linkname[r < strsz ? r : strsz] = '\0';
    fputs(linkname, stdout);
    free(linkname);
}

void process_direntry(DIR *dir, const dirent_t *entry, unsigned int *table) {
    const char *fname = entry->d_name;
    if (!show_all && fname[0] == '.') { 
        return; 
    }

    stat_t s;
    int fd = dirfd(dir);
    if (fd < 0) {
        error(1, errno, "Error while processing %s", fname);
    }
    if (fstatat(fd, fname, &s, AT_SYMLINK_NOFOLLOW)) {
        error(1, errno, "Error while processing %s", fname);
    }
    if (show_long_info) {
        printf("%c%s %*ld %*s %*s %*ld %*s ", 
                filetype_prefix(s.st_mode & S_IFMT), rwx(s.st_mode), 
                table[COL_NLINK], s.st_nlink,
                table[COL_USER],  getpwuid(s.st_uid)->pw_name,
                table[COL_GROUP], getgrgid(s.st_gid)->gr_name,
                table[COL_FSIZE], s.st_size,
                table[COL_CTIME], get_change_time(&s));
    }
    printf("%s%c", fname, filetype_suffix(s.st_mode & S_IFMT));
    if (show_long_info && S_ISLNK(s.st_mode)) {
        printf(" -> ");
        print_symlink(fd, fname);
    }
    printf("\n");
}

void process_dir(const char *dirname) {
    unsigned int table_columns_width[COL_TOTAL_COUNT];
    for (int i = 0; i < COL_TOTAL_COUNT; ++i) {
        table_columns_width[i] = 0;
    }

    DIR *dir;
    dirent_t *entry;

    dir = opendir(dirname);
    if (dir == NULL) {
        error(2, errno, "Cannot open directory %s", dirname);
    }

    if (show_long_info) {
        entry = readdir(dir);
        while (entry != NULL) {
            prepare_table(dir, entry, table_columns_width);
            entry = readdir(dir);
        } 
        if (errno == EBADF) {
            error(1, EBADF, "Invalid directory descriptor");
        }
        rewinddir(dir);
    }

    entry = readdir(dir);
    while (entry != NULL) {
        process_direntry(dir, entry, table_columns_width);
        entry = readdir(dir);
    } 
    if (errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }
    if (closedir(dir) && errno == EBADF) {
        error(1, EBADF, "Invalid directory descriptor");
    }
}

int parse_args(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l':
                show_long_info = true;
                break;
            case 'a':
                show_all = true;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-a] DIR1 [DIR2 ...]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (parse_args(argc, argv) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

#ifdef DEBUG
    printf("Show long info: %d; show all: %d\n", show_long_info, show_all);
#endif

    int processed_dirs_cnt = 0;

    for (int i = optind; i < argc; ++i) {
        printf("%s:\n", argv[i]);
        process_dir(argv[i]);
        ++processed_dirs_cnt;
    }

    if (!processed_dirs_cnt) {
        process_dir("./");
    }

    return EXIT_SUCCESS;
}
