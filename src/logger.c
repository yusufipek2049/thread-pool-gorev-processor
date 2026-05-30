#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static void logger_write(FILE *stream, const char *level, const char *format, va_list args)
{
    char time_buffer[32];
    time_t now;
    struct tm local_time;

    now = time(NULL);
    localtime_r(&now, &local_time);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &local_time);

    fprintf(stream, "[%s] [%s] ", time_buffer, level);
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
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
