#include "metrics.h"

#include <stdio.h>
#include <math.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BOLD    "\x1b[1m"

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
    double throughput = 0.0;
    int success_percent = 0;

    if (g_metrics.total_jobs > 0) {
        average = g_metrics.total_job_time / (double)g_metrics.total_jobs;
        success_percent = (g_metrics.successful_jobs * 100) / g_metrics.total_jobs;
        if (g_metrics.total_job_time > 0.0) {
            throughput = (double)g_metrics.total_jobs / g_metrics.total_job_time;
        }
    }

    printf("\n");
    printf("┌──────────────────────────────────────────────────────┐\n");
    printf("│              📊 PERFORMANS RAPORU 📊               │\n");
    printf("├──────────────────────────────────────────────────────┤\n");
    printf("│ Worker Sayısı:              %d                     │\n", g_metrics.worker_count);
    printf("│ Toplam İş:                  %d                     │\n", g_metrics.total_jobs);
    printf("│ %s✓ Başarılı%s:                  %d                     │\n", 
           ANSI_COLOR_GREEN, ANSI_COLOR_RESET, g_metrics.successful_jobs);
    printf("│ %s✗ Başarısız%s:                  %d                     │\n", 
           ANSI_COLOR_RED, ANSI_COLOR_RESET, g_metrics.failed_jobs);
    printf("│ Başarı Oranı:               %d%%                   │\n", success_percent);
    printf("│ Toplam Süre:                %.4f saniye           │\n", g_metrics.total_job_time);
    printf("│ Ortalama Süre:              %.4f saniye           │\n", average);
    printf("│ İş/Saniye (Throughput):     %.2f İş/s             │\n", throughput);
    printf("└──────────────────────────────────────────────────────┘\n");
    printf("\n");
}

void metrics_destroy(void)
{
    g_metrics.worker_count = 0;
    g_metrics.total_jobs = 0;
    g_metrics.successful_jobs = 0;
    g_metrics.failed_jobs = 0;
    g_metrics.total_job_time = 0.0;
}
