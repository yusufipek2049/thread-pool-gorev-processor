#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BOLD    "\x1b[1m"

static void logger_write(FILE *stream, const char *level, const char *color, const char *format, va_list args)
{
    char time_buffer[32];
    time_t now;
    struct tm local_time;

    now = time(NULL);
    localtime_r(&now, &local_time);
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", &local_time);

    fprintf(stream, "%s[%s]%s %s%-8s%s | ", 
            ANSI_COLOR_CYAN, time_buffer, ANSI_COLOR_RESET,
            color, level, ANSI_COLOR_RESET);
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
}

void logger_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stdout, "INFO", ANSI_COLOR_GREEN, format, args);
    va_end(args);
}

void logger_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stderr, "ERROR", ANSI_COLOR_RED, format, args);
    va_end(args);
}

void logger_debug(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    logger_write(stdout, "DEBUG", ANSI_COLOR_YELLOW, format, args);
    va_end(args);
}
