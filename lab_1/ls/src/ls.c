#include <unistd.h>
#include <stdio.h> 
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <parseargs/parseargs.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define RWX_STRING      "rwxrwxrwx"
#define TIME_FORMAT     "%b %d %Y %H:%M"

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

BEGIN_ARG_ENUM
ADD_ARG('l', show_long_info)
ADD_ARG('a', show_all)
END_ARG_ENUM

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
        case __S_IFCHR: return 'c';
        case __S_IFBLK: return 'b';
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
    if (!show_all && fname[0] == '.') { return; }

    stat_t s;
    fstatat(dirfd(dir), fname, &s, 0);
    table[COL_NLINK] = max_u64(table[COL_NLINK], int_length(s.st_nlink));
    table[COL_USER]  = max_u64(table[COL_USER],  strlen(getpwuid(s.st_uid)->pw_name));
    table[COL_GROUP] = max_u64(table[COL_GROUP], strlen(getgrgid(s.st_gid)->gr_name));
    table[COL_FSIZE] = max_u64(table[COL_FSIZE], int_length(s.st_size));
    table[COL_CTIME] = max_u64(table[COL_CTIME], strlen(get_change_time(&s)));
}

void process_direntry(DIR *dir, const dirent_t *entry, unsigned int *table) {
    const char *fname = entry->d_name;
    if (!show_all && fname[0] == '.') { return; }

    stat_t s;
    fstatat(dirfd(dir), fname, &s, 0);
    if (show_long_info) {
        printf("%c%s %*ld %*s %*s %*ld %*s ", 
                filetype_prefix(s.st_mode & __S_IFMT), rwx(s.st_mode), 
                table[COL_NLINK], s.st_nlink,
                table[COL_USER],  getpwuid(s.st_uid)->pw_name,
                table[COL_GROUP], getgrgid(s.st_gid)->gr_name,
                table[COL_FSIZE], s.st_size,
                table[COL_CTIME], get_change_time(&s));
    }
    printf("%s%c\n", fname, filetype_suffix(s.st_mode & __S_IFMT));
}

void process_dir(const char *dirname, arg_arr_t arg_arr) {
    unsigned int table_columns_width[COL_TOTAL_COUNT];
    for (int i = 0; i < COL_TOTAL_COUNT; ++i) {
        table_columns_width[i] = 0;
    }

    DIR *dir;
    dirent_t *entry;

    if (arg_arr[show_long_info]) {
        dir = opendir(dirname);
        entry = readdir(dir);
        while (entry != NULL) {
            prepare_table(dir, entry, table_columns_width);
            entry = readdir(dir);
        } 
        closedir(dir);
    }

    dir = opendir(dirname);
    entry = readdir(dir);
    while (entry != NULL) {
        process_direntry(dir, entry, table_columns_width);
        entry = readdir(dir);
    } 
    closedir(dir);
}

int main(int argc, char *argv[]) {
    arg_arr_t arg_arr;
    parse_args(argc, argv, arg_arr);

#ifdef DEBUG
    printf("Show long info: %d; show all: %d\n", arg_arr[show_long_info], arg_arr[show_all]);
#endif

    int processed_dirs_cnt = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            printf("%s:\n", argv[i]);
            process_dir(argv[i], arg_arr);
            ++processed_dirs_cnt;
        }
    }

    if (!processed_dirs_cnt) {
        process_dir("./", arg_arr);
    }

    return 0;
}
