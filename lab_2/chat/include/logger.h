#ifndef XENON_CHAT_LOGGER_H
#define XENON_CHAT_LOGGER_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define __strftime_buffer_size__ 80

static time_t __cur_time__;
static char __strftime_buffer__[__strftime_buffer_size__];
static int __strftime_strlen__;

static const char *__short_file_name__(const char *fname) {
    const char *ptr = strrchr(fname, '/');
    return (ptr == NULL) ? (fname) : (ptr + 1);
}

void logger_lock();
void logger_unlock();

#define LOG_PRE     time(&__cur_time__); \
                    __strftime_strlen__ = strftime(__strftime_buffer__, \
                            __strftime_buffer_size__, \
                            "[%F %T]", localtime(&__cur_time__)); \
                    fprintf(stderr, "%.*s ", __strftime_strlen__, \
                            __strftime_buffer__);

#ifdef DEBUG
#define LOG_MODULE  fprintf(stderr, "<at %s:%d> ", __short_file_name__(__FILE__), __LINE__);
#else
#ifndef LOG_MODULE_NAME
#define LOG_MODULE_NAME __short_file_name__(__FILE__)
#endif
#define LOG_MODULE  fprintf(stderr, "[%s] ", LOG_MODULE_NAME);
#endif

#define LOG(...)    logger_lock(); \
                    LOG_PRE; \
                    LOG_MODULE; \
                    fprintf(stderr, ##__VA_ARGS__); \
                    fprintf(stderr, "\n"); \
                    logger_unlock();

#endif /* XENON_CHAT_LOGGER_H */
