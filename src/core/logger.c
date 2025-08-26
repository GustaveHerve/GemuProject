#include "logger.h"

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define COLOR_RESET "\x1b[0m"

#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

static enum log_verbosity max_verbosity = DEBUG;

int logger_set_verbosity(enum log_verbosity verbosity)
{
    if (verbosity > DEBUG)
        return EXIT_FAILURE;
    max_verbosity = verbosity;
    return EXIT_SUCCESS;
}

int logger_log(enum log_verbosity verbosity, const char *format, ...)
{
    if (verbosity > max_verbosity)
        return EXIT_FAILURE;
    static const char *log_headers[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    static const char *log_colors[] = {COLOR_RED, COLOR_YELLOW, COLOR_RESET, COLOR_BLUE};
    assert(verbosity < sizeof(log_headers) / sizeof(char *));

    pthread_mutex_lock(&log_mutex);
    fprintf(stderr, "%s[%s]%s ", log_colors[verbosity], log_headers[verbosity], COLOR_RESET);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);
    fflush(stderr);
    pthread_mutex_unlock(&log_mutex);

    return EXIT_SUCCESS;
}
