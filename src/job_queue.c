#include "job_queue.h"

#include <pthread.h>
#include <stdlib.h>

struct job_queue {
    job_t *items;
    int capacity;
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
};

job_queue_t *job_queue_create(int capacity)
{
    job_queue_t *queue;

    if (capacity <= 0) {
        return NULL;
    }

    queue = malloc(sizeof(*queue));
    if (queue == NULL) {
        return NULL;
    }

    queue->items = calloc((size_t)capacity, sizeof(job_t));
    if (queue->items == NULL) {
        free(queue);
        return NULL;
    }

    queue->capacity = capacity;
    queue->size = 0;
    queue->front = 0;
    queue->rear = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->items);
        free(queue);
        return NULL;
    }

    return queue;
}

void job_queue_destroy(job_queue_t *queue)
{
    if (queue == NULL) {
        return;
    }

    pthread_mutex_destroy(&queue->mutex);
    free(queue->items);
    free(queue);
}

int job_queue_push(job_queue_t *queue, job_t job)
{
    int result = 0;

    if (queue == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        return -1;
    }

    if (queue->size == queue->capacity) {
        result = -1;
    } else {
        queue->items[queue->rear] = job;
        queue->rear = (queue->rear + 1) % queue->capacity;
        queue->size++;
    }

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        return -1;
    }

    return result;
}

int job_queue_pop(job_queue_t *queue, job_t *job)
{
    int result = 0;

    if (queue == NULL || job == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        return -1;
    }

    if (queue->size == 0) {
        result = -1;
    } else {
        *job = queue->items[queue->front];
        queue->front = (queue->front + 1) % queue->capacity;
        queue->size--;
    }

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        return -1;
    }

    return result;
}

int job_queue_is_empty(job_queue_t *queue)
{
    int is_empty;

    if (queue == NULL) {
        return 1;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        return 1;
    }

    is_empty = queue->size == 0;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        return 1;
    }

    return is_empty;
}

int job_queue_is_full(job_queue_t *queue)
{
    int is_full;

    if (queue == NULL) {
        return 0;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        return 0;
    }

    is_full = queue->size == queue->capacity;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        return 0;
    }

    return is_full;
}

int job_queue_size(job_queue_t *queue)
{
    int size;

    if (queue == NULL) {
        return 0;
    }

    if (pthread_mutex_lock(&queue->mutex) != 0) {
        return 0;
    }

    size = queue->size;

    if (pthread_mutex_unlock(&queue->mutex) != 0) {
        return 0;
    }

    return size;
}

int job_queue_capacity(job_queue_t *queue)
{
    if (queue == NULL) {
        return 0;
    }

    return queue->capacity;
}
