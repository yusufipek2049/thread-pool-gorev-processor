#include "metrics.h"

#include <stdio.h>

typedef struct {
    int worker_count;
    int total_jobs;
    int successful_jobs;
    int failed_jobs;
    double total_job_time;
} metrics_state_t;

static metrics_state_t g_metrics;

void metrics_init(int worker_count)
{
    g_metrics.worker_count = worker_count;
    g_metrics.total_jobs = 0;
    g_metrics.successful_jobs = 0;
    g_metrics.failed_jobs = 0;
    g_metrics.total_job_time = 0.0;
}

void metrics_record_job_success(double duration_seconds)
{
    g_metrics.total_jobs++;
    g_metrics.successful_jobs++;
    g_metrics.total_job_time += duration_seconds;
}

void metrics_record_job_failure(double duration_seconds)
{
    g_metrics.total_jobs++;
    g_metrics.failed_jobs++;
    g_metrics.total_job_time += duration_seconds;
}

void metrics_print_report(void)
{
    double average = 0.0;

    if (g_metrics.total_jobs > 0) {
        average = g_metrics.total_job_time / (double)g_metrics.total_jobs;
    }

    printf("\nPerformans Raporu\n");
    printf("-----------------\n");
    printf("Worker sayisi: %d\n", g_metrics.worker_count);
    printf("Toplam is sayisi: %d\n", g_metrics.total_jobs);
    printf("Basarili is sayisi: %d\n", g_metrics.successful_jobs);
    printf("Basarisiz is sayisi: %d\n", g_metrics.failed_jobs);
    printf("Toplam is suresi: %.6f saniye\n", g_metrics.total_job_time);
    printf("Ortalama is suresi: %.6f saniye\n\n", average);
}

void metrics_destroy(void)
{
    g_metrics.worker_count = 0;
    g_metrics.total_jobs = 0;
    g_metrics.successful_jobs = 0;
    g_metrics.failed_jobs = 0;
    g_metrics.total_job_time = 0.0;
}
