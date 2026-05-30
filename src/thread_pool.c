#include "thread_pool.h"

#include "job_queue.h"
#include "logger.h"
#include "metrics.h"
#include "tasks.h"

#include <stdlib.h>
#include <time.h>

struct thread_pool {
    int worker_count;
    int is_shutdown;
    job_queue_t *queue;
};

static double elapsed_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

thread_pool_t *thread_pool_create(int worker_count, int queue_size)
{
    thread_pool_t *pool;

    if (worker_count <= 0 || queue_size <= 0) {
        return NULL;
    }

    pool = malloc(sizeof(*pool));
    if (pool == NULL) {
        return NULL;
    }

    pool->queue = job_queue_create(queue_size);
    if (pool->queue == NULL) {
        free(pool);
        return NULL;
    }

    pool->worker_count = worker_count;
    pool->is_shutdown = 0;
    metrics_init(worker_count);

    logger_info("Thread pool created: workers=%d, queue_size=%d",
                worker_count,
                queue_size);

    return pool;
}

int thread_pool_submit(thread_pool_t *pool, job_t job)
{
    job_t queued_job;
    int result;
    struct timespec start;
    struct timespec end;
    double duration;

    if (pool == NULL || pool->is_shutdown) {
        return -1;
    }

    if (job_queue_push(pool->queue, job) != 0) {
        logger_error("Job-%d could not be queued", job.id);
        return -1;
    }

    logger_info("Job-%d queued: %s", job.id, job_type_to_string(job.type));

    if (job_queue_pop(pool->queue, &queued_job) != 0) {
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    result = execute_job(&queued_job);
    clock_gettime(CLOCK_MONOTONIC, &end);

    duration = elapsed_seconds(start, end);
    if (result == 0) {
        metrics_record_job_success(duration);
    } else {
        metrics_record_job_failure(duration);
    }

    return result;
}

void thread_pool_shutdown(thread_pool_t *pool)
{
    if (pool == NULL) {
        return;
    }

    pool->is_shutdown = 1;
    logger_info("Thread pool shutdown requested");
    metrics_print_report();
}

void thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == NULL) {
        return;
    }

    job_queue_destroy(pool->queue);
    metrics_destroy();
    logger_info("Thread pool destroyed");
    free(pool);
}
