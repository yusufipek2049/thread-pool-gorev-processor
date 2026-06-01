#include "tasks.h"

#include "logger.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int is_prime(long number)
{
    long divisor;

    if (number < 2) {
        return 0;
    }

    if (number == 2) {
        return 1;
    }

    if (number % 2 == 0) {
        return 0;
    }

    for (divisor = 3; divisor <= number / divisor; divisor += 2) {
        if (number % divisor == 0) {
            return 0;
        }
    }

    return 1;
}

static int count_lines(const char *path, long *line_count)
{
    FILE *file;
    int character;

    file = fopen(path, "r");
    if (file == NULL) {
        logger_error("Could not open file '%s': %s", path, strerror(errno));
        return -1;
    }

    *line_count = 0;
    while ((character = fgetc(file)) != EOF) {
        if (character == '\n') {
            (*line_count)++;
        }
    }

    fclose(file);
    return 0;
}

static int hash_file(const char *path, unsigned long *hash)
{
    FILE *file;
    int character;

    file = fopen(path, "rb");
    if (file == NULL) {
        logger_error("Could not open file '%s': %s", path, strerror(errno));
        return -1;
    }

    *hash = 5381;
    while ((character = fgetc(file)) != EOF) {
        *hash = ((*hash << 5) + *hash) + (unsigned long)character;
    }

    fclose(file);
    return 0;
}

int execute_job(const job_t *job)
{
    long line_count;
    unsigned long hash;
    int prime;

    if (job == NULL) {
        return -1;
    }

    switch (job->type) {
    case JOB_PRIME_CHECK:
        prime = is_prime(job->number);
        logger_info("Job-%d completed: number=%ld, prime=%s",
                    job->id,
                    job->number,
                    prime ? "true" : "false");
        return 0;
    case JOB_LINE_COUNT:
        if (count_lines(job->path, &line_count) != 0) {
            return -1;
        }
        logger_info("Job-%d completed: file='%s', lines=%ld",
                    job->id,
                    job->path,
                    line_count);
        return 0;
    case JOB_FILE_HASH:
        if (hash_file(job->path, &hash) != 0) {
            return -1;
        }
        logger_info("Job-%d completed: file='%s', hash=%lu",
                    job->id,
                    job->path,
                    hash);
        return 0;
    default:
        logger_error("Job-%d failed: unsupported job type", job->id);
        return -1;
    }
}
