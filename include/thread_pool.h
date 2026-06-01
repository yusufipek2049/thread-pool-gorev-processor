#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "job.h"

typedef struct thread_pool thread_pool_t;

thread_pool_t *thread_pool_create(int worker_count, int queue_size);
int thread_pool_submit(thread_pool_t *pool, job_t job);
void thread_pool_shutdown(thread_pool_t *pool);
void thread_pool_destroy(thread_pool_t *pool);

#endif
