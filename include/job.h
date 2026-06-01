#ifndef JOB_H
#define JOB_H

#define MAX_PATH_LEN 256

typedef enum {
    JOB_FILE_HASH = 1,
    JOB_LINE_COUNT = 2,
    JOB_PRIME_CHECK = 3
} job_type_t;

typedef struct {
    int id;
    job_type_t type;
    char path[MAX_PATH_LEN];
    long number;
} job_t;

const char *job_type_to_string(job_type_t type);

#endif
