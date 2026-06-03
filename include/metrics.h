#ifndef METRICS_H
#define METRICS_H

void metrics_init(int worker_count);
void metrics_record_job_success(double duration_seconds);
void metrics_record_job_failure(double duration_seconds);
void metrics_print_report(int max_queue_size, int queue_capacity);
void metrics_destroy(void);

#endif
