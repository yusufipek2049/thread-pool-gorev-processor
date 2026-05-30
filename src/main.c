#include "job.h"
#include "logger.h"
#include "thread_pool.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int worker_count = 4;
    int queue_size = 32;
    thread_pool_t *pool;
    job_t jobs[] = {
        {1, JOB_PRIME_CHECK, "", 999983},
        {2, JOB_LINE_COUNT, "tests/sample1.txt", 0},
        {3, JOB_FILE_HASH, "tests/sample2.txt", 0},
        {4, JOB_PRIME_CHECK, "", 1234567}
    };
    size_t job_count = sizeof(jobs) / sizeof(jobs[0]);
    size_t index;

    logger_info("Program started");

    if (argc > 1) {
        printf("Komut satiri argumanlari sonraki asamada islenecek.\n");
        printf("Ilk arguman: %s\n", argv[1]);
    }

    pool = thread_pool_create(worker_count, queue_size);
    if (pool == NULL) {
        logger_error("Thread pool could not be created");
        return 1;
    }

    for (index = 0; index < job_count; index++) {
        if (thread_pool_submit(pool, jobs[index]) != 0) {
            logger_error("Job-%d failed or could not be submitted", jobs[index].id);
        }
    }

    thread_pool_shutdown(pool);
    thread_pool_destroy(pool);

    logger_info("Program finished");

    return 0;
}
