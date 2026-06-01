#ifndef METRICS_H
#define METRICS_H

void metrics_init(int worker_count);
void metrics_record_job_success(double duration_seconds);
void metrics_record_job_failure(double duration_seconds);
void metrics_print_report(void);
void metrics_destroy(void);

#endif
