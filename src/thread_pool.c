#include "thread_pool.h"

#include "job_queue.h"
#include "logger.h"
#include "metrics.h"
#include "tasks.h"

#include <pthread.h>
#include <stdlib.h>
#include <time.h>

struct thread_pool {
    pthread_t *workers;
    int worker_count;
    int is_shutdown;
    job_queue_t *queue;
    pthread_mutex_t shutdown_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
};

static double elapsed_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

static void *worker_thread_function(void *arg)
{
    thread_pool_t *pool = (thread_pool_t *)arg;
    job_t job;
    int result;
    struct timespec start;
    struct timespec end;
    double duration;

    logger_debug("Worker thread started");

    while (1) {
        pthread_mutex_lock(&pool->shutdown_mutex);

        while (job_queue_is_empty(pool->queue) && !pool->is_shutdown) {
            pthread_cond_wait(&pool->queue_not_empty, &pool->shutdown_mutex);
        }

        if (pool->is_shutdown && job_queue_is_empty(pool->queue)) {
            pthread_mutex_unlock(&pool->shutdown_mutex);
            break;
        }

        if (job_queue_pop(pool->queue, &job) != 0) {
            pthread_mutex_unlock(&pool->shutdown_mutex);
            continue;
        }

        pthread_cond_signal(&pool->queue_not_full);
        pthread_mutex_unlock(&pool->shutdown_mutex);

        logger_info("Worker processing: Job-%d (%s)",
                    job.id,
                    job_type_to_string(job.type));

        clock_gettime(CLOCK_MONOTONIC, &start);
        result = execute_job(&job);
        clock_gettime(CLOCK_MONOTONIC, &end);

        duration = elapsed_seconds(start, end);
        if (result == 0) {
            logger_info("Job-%d completed successfully (%.3f s)", job.id, duration);
            metrics_record_job_success(duration);
        } else {
            logger_error("Job-%d failed (%.3f s)", job.id, duration);
            metrics_record_job_failure(duration);
        }
    }

    logger_debug("Worker thread exiting");
    return NULL;
}

thread_pool_t *thread_pool_create(int worker_count, int queue_size)
{
    thread_pool_t *pool;
    int i;
    int error;

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

    pool->workers = malloc((size_t)worker_count * sizeof(pthread_t));
    if (pool->workers == NULL) {
        job_queue_destroy(pool->queue);
        free(pool);
        return NULL;
    }

    pool->worker_count = worker_count;
    pool->is_shutdown = 0;

    if (pthread_mutex_init(&pool->shutdown_mutex, NULL) != 0) {
        free(pool->workers);
        job_queue_destroy(pool->queue);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->queue_not_empty, NULL) != 0) {
        pthread_mutex_destroy(&pool->shutdown_mutex);
        free(pool->workers);
        job_queue_destroy(pool->queue);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->queue_not_full, NULL) != 0) {
        pthread_cond_destroy(&pool->queue_not_empty);
        pthread_mutex_destroy(&pool->shutdown_mutex);
        free(pool->workers);
        job_queue_destroy(pool->queue);
        free(pool);
        return NULL;
    }

    metrics_init(worker_count);

    for (i = 0; i < worker_count; i++) {
        error = pthread_create(&pool->workers[i], NULL,
                               worker_thread_function, pool);
        if (error != 0) {
            logger_error("Failed to create worker thread %d", i);
            pool->worker_count = i;
            thread_pool_shutdown(pool);
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    logger_info("Thread pool created: workers=%d, queue_size=%d",
                worker_count,
                queue_size);

    return pool;
}

int thread_pool_submit(thread_pool_t *pool, job_t job)
{
    if (pool == NULL) {
        return -1;
    }

    pthread_mutex_lock(&pool->shutdown_mutex);

    while (!pool->is_shutdown && job_queue_is_full(pool->queue)) {
        pthread_cond_wait(&pool->queue_not_full, &pool->shutdown_mutex);
    }

    if (pool->is_shutdown) {
        pthread_mutex_unlock(&pool->shutdown_mutex);
        logger_error("Job-%d could not be queued: pool is shutting down", job.id);
        return -1;
    }

    if (job_queue_push(pool->queue, job) != 0) {
        pthread_mutex_unlock(&pool->shutdown_mutex);
        logger_error("Job-%d could not be queued", job.id);
        return -1;
    }

    logger_info("Job-%d submitted to queue: %s", job.id, job_type_to_string(job.type));
    pthread_cond_signal(&pool->queue_not_empty);
    pthread_mutex_unlock(&pool->shutdown_mutex);

    return 0;
}

void thread_pool_shutdown(thread_pool_t *pool)
{
    int i;

    if (pool == NULL) {
        return;
    }

    logger_info("Thread pool shutdown requested");

    pthread_mutex_lock(&pool->shutdown_mutex);
    pool->is_shutdown = 1;
    pthread_cond_broadcast(&pool->queue_not_empty);
    pthread_cond_broadcast(&pool->queue_not_full);
    pthread_mutex_unlock(&pool->shutdown_mutex);

    for (i = 0; i < pool->worker_count; i++) {
        pthread_join(pool->workers[i], NULL);
        logger_debug("Worker thread %d joined", i);
    }

    logger_info("All worker threads have finished");
    metrics_print_report();
}

void thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == NULL) {
        return;
    }

    pthread_cond_destroy(&pool->queue_not_empty);
    pthread_cond_destroy(&pool->queue_not_full);
    pthread_mutex_destroy(&pool->shutdown_mutex);
    job_queue_destroy(pool->queue);
    metrics_destroy();
    free(pool->workers);
    logger_info("Thread pool destroyed");
    free(pool);
}
