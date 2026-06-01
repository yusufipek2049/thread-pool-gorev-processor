#include "job.h"

const char *job_type_to_string(job_type_t type)
{
    switch (type) {
    case JOB_FILE_HASH:
        return "FILE_HASH";
    case JOB_LINE_COUNT:
        return "LINE_COUNT";
    case JOB_PRIME_CHECK:
        return "PRIME_CHECK";
    default:
        return "UNKNOWN";
    }
}
