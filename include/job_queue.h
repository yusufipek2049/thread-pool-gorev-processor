#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "job.h"

typedef struct job_queue job_queue_t;

job_queue_t *job_queue_create(int capacity);
void job_queue_destroy(job_queue_t *queue);

int job_queue_push(job_queue_t *queue, job_t job);
int job_queue_pop(job_queue_t *queue, job_t *job);

int job_queue_is_empty(job_queue_t *queue);
int job_queue_is_full(job_queue_t *queue);
int job_queue_size(job_queue_t *queue);
int job_queue_capacity(job_queue_t *queue);

#endif
