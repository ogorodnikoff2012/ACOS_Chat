#include <common/logger.h>
#include <pthread.h>

static pthread_mutex_t logger_mutex;

__attribute__((constructor))
static void logger_init() {
    pthread_mutex_init(&logger_mutex, NULL);
}

__attribute__((destructor))
static void logger_destroy() {
    pthread_mutex_destroy(&logger_mutex);
}

void logger_lock() {
    pthread_mutex_lock(&logger_mutex);
}

void logger_unlock() {
    pthread_mutex_unlock(&logger_mutex);
}
