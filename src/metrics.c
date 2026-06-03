#include "metrics.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    int worker_count;
    int total_jobs;
    int successful_jobs;
    int failed_jobs;
    double total_job_time;
    struct timespec start_time;
} metrics_state_t;

static metrics_state_t g_metrics;
static pthread_mutex_t g_metrics_mutex = PTHREAD_MUTEX_INITIALIZER;

void metrics_init(int worker_count)
{
    pthread_mutex_lock(&g_metrics_mutex);
    g_metrics.worker_count = worker_count;
    g_metrics.total_jobs = 0;
    g_metrics.successful_jobs = 0;
    g_metrics.failed_jobs = 0;
    g_metrics.total_job_time = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &g_metrics.start_time);
    pthread_mutex_unlock(&g_metrics_mutex);
}

void metrics_record_job_success(double duration_seconds)
{
    pthread_mutex_lock(&g_metrics_mutex);
    g_metrics.total_jobs++;
    g_metrics.successful_jobs++;
    g_metrics.total_job_time += duration_seconds;
    pthread_mutex_unlock(&g_metrics_mutex);
}

void metrics_record_job_failure(double duration_seconds)
{
    pthread_mutex_lock(&g_metrics_mutex);
    g_metrics.total_jobs++;
    g_metrics.failed_jobs++;
    g_metrics.total_job_time += duration_seconds;
    pthread_mutex_unlock(&g_metrics_mutex);
}

void metrics_print_report(int max_queue_size, int queue_capacity)
{
    metrics_state_t snapshot;
    double average = 0.0;
    double throughput = 0.0;
    int success_percent = 0;
    struct timespec end_time;
    double elapsed_time = 0.0;
    double thread_utilization = 0.0;

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    pthread_mutex_lock(&g_metrics_mutex);
    snapshot = g_metrics;
    pthread_mutex_unlock(&g_metrics_mutex);

    elapsed_time = (double)(end_time.tv_sec - snapshot.start_time.tv_sec) +
                   (double)(end_time.tv_nsec - snapshot.start_time.tv_nsec) / 1000000000.0;

    if (snapshot.total_jobs > 0) {
        average = snapshot.total_job_time / (double)snapshot.total_jobs;
        success_percent = (snapshot.successful_jobs * 100) / snapshot.total_jobs;
        if (snapshot.total_job_time > 0.0) {
            throughput = (double)snapshot.total_jobs / snapshot.total_job_time;
        }
    }

    if (elapsed_time > 0.0 && snapshot.worker_count > 0) {
        thread_utilization = (snapshot.total_job_time / (snapshot.worker_count * elapsed_time)) * 100.0;
        if (thread_utilization > 100.0) {
            thread_utilization = 100.0;
        }
    }

    double queue_fullness_percent = 0.0;
    if (queue_capacity > 0) {
        queue_fullness_percent = ((double)max_queue_size / queue_capacity) * 100.0;
    }

    printf("\n");
    printf("Performans Raporu\n");
    printf("-----------------\n");
    printf("Worker sayisi: %d\n", snapshot.worker_count);
    printf("Toplam is: %d\n", snapshot.total_jobs);
    printf("Basarili is: %d\n", snapshot.successful_jobs);
    printf("Basarisiz is: %d\n", snapshot.failed_jobs);
    printf("Basari orani: %d%%\n", success_percent);
    printf("Toplam is suresi: %.4f saniye\n", snapshot.total_job_time);
    printf("Toplam gecen sure (duvar saati): %.4f saniye\n", elapsed_time);
    printf("Thread kullanim orani: %.2f%%\n", thread_utilization);
    printf("En yuksek kuyruk dolulugu: %d/%d (%.2f%%)\n", max_queue_size, queue_capacity, queue_fullness_percent);
    printf("Ortalama is suresi: %.4f saniye\n", average);
    printf("Is/saniye: %.2f\n", throughput);
    printf("\n");
}

void metrics_destroy(void)
{
    pthread_mutex_lock(&g_metrics_mutex);
    g_metrics.worker_count = 0;
    g_metrics.total_jobs = 0;
    g_metrics.successful_jobs = 0;
    g_metrics.failed_jobs = 0;
    g_metrics.total_job_time = 0.0;
    pthread_mutex_unlock(&g_metrics_mutex);
}
