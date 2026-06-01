#include "metrics.h"

#include <pthread.h>
#include <stdio.h>

typedef struct {
    int worker_count;
    int total_jobs;
    int successful_jobs;
    int failed_jobs;
    double total_job_time;
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

void metrics_print_report(void)
{
    metrics_state_t snapshot;
    double average = 0.0;
    double throughput = 0.0;
    int success_percent = 0;

    pthread_mutex_lock(&g_metrics_mutex);
    snapshot = g_metrics;
    pthread_mutex_unlock(&g_metrics_mutex);

    if (snapshot.total_jobs > 0) {
        average = snapshot.total_job_time / (double)snapshot.total_jobs;
        success_percent = (snapshot.successful_jobs * 100) / snapshot.total_jobs;
        if (snapshot.total_job_time > 0.0) {
            throughput = (double)snapshot.total_jobs / snapshot.total_job_time;
        }
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
