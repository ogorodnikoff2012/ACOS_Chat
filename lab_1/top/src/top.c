#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <tableprint/tableprint.h>
#include <dirent.h>
#include <pwd.h>

#define PROCINFO_PID_NOT_FOUND -2
#define PROCINFO_FAILURE -1

typedef struct {
    char stat;
    int pid, ppid, uid;
    long nice, rss;
    unsigned long utime, stime;
    long long starttime;
} procinfo_t;

int get_ram_in_kb(void) {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL) {
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), meminfo)) {
        int ram;
        if (sscanf(line, "MemTotal: %d kB", &ram) == 1) {
            fclose(meminfo);
            return ram;
        }
    }

    fclose(meminfo);
    return -1;
}

procinfo_t get_procinfo(int pid) {
    procinfo_t res;
    char pstat_fname[80];
    snprintf(pstat_fname, 80, "/proc/%d/stat", pid);

    FILE *pstat = fopen(pstat_fname, "r");
    if (pstat == NULL) {
        res.pid = PROCINFO_PID_NOT_FOUND;
        return res;
    }
    char stat_buf[4096];
    fread(stat_buf, 4096, 1, pstat);
    fclose(pstat);

    sscanf(stat_buf, "%d", &res.pid);
    char *stat_skip_comm = strrchr(stat_buf, ')');
    if (stat_skip_comm == NULL) {
        res.pid = PROCINFO_FAILURE;
        return res;
    } else {
        stat_skip_comm += 2;
    }

    sscanf(stat_skip_comm, "%c %d %*d %*d %*d %*d %*u %*lu %*lu %*lu "
            "%*lu %lu %lu %*ld %*ld %*ld %ld %*ld %*ld %llu %*lu %ld",
            &res.stat, &res.ppid, &res.utime, &res.stime, &res.nice, &res.starttime, &res.rss);

    struct stat procstat_stat;
    if (stat(pstat_fname, &procstat_stat) != 0) {
        res.uid = PROCINFO_FAILURE;
        return res;
    }

    res.uid = procstat_stat.st_uid;
    return res;
}

long long get_starttime(void) {
    return get_procinfo(getpid()).starttime;
}

int read_comm(int pid, char *buf, int length) {
    if (length < 1) {
        return 0;
    }
    char pstat_fname[80];
    snprintf(pstat_fname, 80, "/proc/%d/comm", pid);

    FILE *pstat = fopen(pstat_fname, "r");
    if (pstat == NULL) {
        buf[0] = 0;
        return 0;
    }
    int read_len = fread(buf, 1, length - 1, pstat);
    read_len--;
    buf[read_len] = 0;
    fclose(pstat);
    return read_len;
}

int read_cmdline(int pid, char *buf, int length) {
    if (length < 1) {
        return 0;
    }
    char pstat_fname[80];
    snprintf(pstat_fname, 80, "/proc/%d/cmdline", pid);

    FILE *pstat = fopen(pstat_fname, "r");
    if (pstat == NULL) {
        buf[0] = 0;
        return 0;
    }
    int read_len = fread(buf, 1, length - 1, pstat);
    buf[read_len] = 0;
    fclose(pstat);
    return read_len;
}

static inline long long max_i64(long long a, long long b) {
    return b < a ? a : b;
}

int main(void) {
    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        error(1, errno, "Error while opening /proc");
    }

    table_t table;
    init_table(&table, 9);
    table_set_formatter(&table, 8, "%-*.60s");

    long long starttime = get_starttime();
    long long memsize = get_ram_in_kb() * 1024ULL;
    double clock_ticks = sysconf(_SC_CLK_TCK);
    long long pagesize = sysconf(_SC_PAGESIZE);
    char buf[4096];

    add_row(&table, "PID", "PPID", "USER", "NICE", "\%CPU", "TIME", "\%MEM", "STAT", "COMMAND");

    struct dirent *entry;
    while ((entry = readdir(proc)) != NULL) {
        int pid = atoi(entry->d_name);
        if (pid > 0) {
            procinfo_t pinfo = get_procinfo(pid);
            if (pinfo.pid < 0) {
                continue;
            }

            struct passwd *pw;
            pw = getpwuid(pinfo.uid);

            double proc_seconds = (pinfo.utime + pinfo.stime) / clock_ticks;
            int run_seconds = max_i64(0, starttime - pinfo.starttime) / clock_ticks;
            double pcpu = 0;
            if (run_seconds > 0) {
                pcpu = proc_seconds / run_seconds * 100;
            }

            double pmem = (double) (pinfo.rss * pagesize) / (memsize);

            int hrs, mins, secs;
            secs = run_seconds % 60;
            mins = (run_seconds / 60) % 60;
            hrs = run_seconds / 3600;

            int pid_pos, ppid_pos, uname_pos, nice_pos, pcpu_pos, time_pos, pmem_pos, stat_pos, end_pos;

            sprintf(buf, "%n%d%c%n%d%c%n%s%c%n%ld%c%n%.2f%c%n%d:%02d:%02d%c%n%.2f%c%n%c%c%n",
                    &pid_pos, pinfo.pid, '\0',
                    &ppid_pos, pinfo.ppid, '\0',
                    &uname_pos, pw->pw_name, '\0',
                    &nice_pos, pinfo.nice, '\0',
                    &pcpu_pos, pcpu, '\0',
                    &time_pos, hrs, mins, secs, '\0',
                    &pmem_pos, pmem, '\0',
                    &stat_pos, pinfo.stat, '\0',
                    &end_pos);
            int cmdlen = read_cmdline(pinfo.pid, buf + end_pos, 4096 - end_pos);
            if (cmdlen == 0) {
                buf[end_pos] = '[';
                int commlen = read_comm(pinfo.pid, buf + end_pos + 1, 4096 - end_pos - 1);
                buf[end_pos + commlen + 1] = ']';
                buf[end_pos + commlen + 2] = '\0';
            }
            add_row(&table, buf + pid_pos, buf + ppid_pos, buf + uname_pos, buf + nice_pos, buf + pcpu_pos,
                    buf + time_pos, buf + pmem_pos, buf + stat_pos, buf + end_pos);
        }
    }

    table.col_widths[8] = 0;

    print_table(&table);
    free_table(&table);
    closedir(proc);
    return 0;
}
