#include "logger.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static pthread_mutex_t g_logger_mutex = PTHREAD_MUTEX_INITIALIZER;

static void logger_write(FILE *stream, const char *level, const char *format, va_list args)
{
    char time_buffer[32];
    time_t now;
    struct tm local_time;

    now = time(NULL);
#ifdef _WIN32
    localtime_s(&local_time, &now);
#else
    localtime_r(&now, &local_time);
#endif
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &local_time);

    pthread_mutex_lock(&g_logger_mutex);
    fprintf(stream, "[%s] [%s] ", time_buffer, level);
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
    fflush(stream);
    pthread_mutex_unlock(&g_logger_mutex);
}

void logger_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stdout, "INFO", format, args);
    va_end(args);
}

void logger_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stderr, "ERROR", format, args);
    va_end(args);
}

void logger_debug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stdout, "DEBUG", format, args);
    va_end(args);
}
