#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

enum log_verbosity
{
    ERROR,
    WARN,
    INFO,
    DEBUG,
};

int logger_set_verbosity(enum log_verbosity verbosity);

int logger_log(enum log_verbosity verbosity, const char *fmt, ...);

#define LOG_ERROR(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        logger_log(ERROR, __VA_ARGS__);                                                                                \
    } while (0)

#define LOG_WARN(...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        logger_log(WARN, __VA_ARGS__);                                                                                 \
    } while (0)

#define LOG_INFO(...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        logger_log(INFO, __VA_ARGS__);                                                                                 \
    } while (0)

#define LOG_DEBUG(...)                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        logger_log(DEBUG, __VA_ARGS__);                                                                                \
    } while (0)

#endif
