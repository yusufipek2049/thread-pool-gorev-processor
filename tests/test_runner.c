#include "job.h"
#include "job_queue.h"
#include "tasks.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test_func) \
    do { \
        printf("Running " #test_func "...\n"); \
        tests_run++; \
        if (test_func()) { \
            tests_passed++; \
            printf("[PASS] " #test_func "\n"); \
        } else { \
            printf("[FAIL] " #test_func "\n"); \
        } \
    } while (0)

int test_job_queue_basic(void)
{
    job_queue_t *q = job_queue_create(2);
    if (!q) return 0;

    if (!job_queue_is_empty(q)) return 0;
    

    // Boş kuyruğa pop yapmaya çalışmak başarısız olmalı
     job_t j_out;
    if (job_queue_pop(q, &j_out) == 0) return 0;

    job_t j1 = {1, JOB_PRIME_CHECK, "", 7};
    job_t j2 = {2, JOB_PRIME_CHECK, "", 11};

    if (job_queue_push(q, j1) != 0) return 0;
    if (job_queue_is_empty(q)) return 0;
    if (job_queue_push(q, j2) != 0) return 0;
    if (!job_queue_is_full(q)) return 0;

    // Kuyruk doluyken yeni bir iş eklemeye çalışmak başarısız olmalı
    job_t j3 = {3, JOB_PRIME_CHECK, "", 13};
     if (job_queue_push(q, j3) == 0) return 0;

    // İşleri sırayla poplanıyor mu kontrol
    if (job_queue_pop(q, &j_out) != 0) return 0;
    if (j_out.id != 1) return 0;
    
    if (job_queue_pop(q, &j_out) != 0) return 0;
    if (j_out.id != 2) return 0;
    
    if (!job_queue_is_empty(q)) return 0;

    job_queue_destroy(q);
    return 1;
}

int test_prime_check(void)
{
    // Asal sayı testi
    job_t j1 = {1, JOB_PRIME_CHECK, "", 17};
    if (execute_job(&j1) != 0) return 0;
    
    // Asal olmayan sayı testi
    job_t j2 = {2, JOB_PRIME_CHECK, "", 15};
    if (execute_job(&j2) != 0) return 0; 
    
    // Negatif sayı, 0 ve 1 için asal olmayan testi
    job_t j3 = {3, JOB_PRIME_CHECK, "", -5};
    if (execute_job(&j3) != 0) return 0;
    
    job_t j4 = {4, JOB_PRIME_CHECK, "", 0};
     if (execute_job(&j4) != 0) return 0;

    job_t j5 = {5, JOB_PRIME_CHECK, "", 1};
    if (execute_job(&j5) != 0) return 0;

    job_t j6 = {6, JOB_PRIME_CHECK, "", 2};
    if (execute_job(&j6) != 0) return 0;
    
    return 1;
}

int test_line_count(void)
{
    job_t j1 = {1, JOB_LINE_COUNT, "tests/sample1.txt", 0};
    FILE *f = fopen("tests/sample1.txt", "w");
    if (f) {
        fprintf(f, "line1\nline2\nline3\n");
        fclose(f);
    }
    if (execute_job(&j1) != 0) return 0;
    
    // Boş dosya testi
    job_t j2 = {2, JOB_LINE_COUNT, "tests/sample_empty.txt", 0};
    FILE *f_empty = fopen("tests/sample_empty.txt", "w");
    if (f_empty) {
        fclose(f_empty);
    }
    if (execute_job(&j2) != 0) return 0;

    // Var olmayan dosya testi - execute_job başarısız olmalı
    job_t j3 = {3, JOB_LINE_COUNT, "tests/non_existent_file.txt", 0};
    if (execute_job(&j3) == 0) return 0;
    
    return 1;
}

int test_file_hash(void)
{
    job_t j1 = {1, JOB_FILE_HASH, "tests/sample1.txt", 0};
    if (execute_job(&j1) != 0) return 0;
    
    // Boş dosya testi
    job_t j2 = {2, JOB_FILE_HASH, "tests/sample_empty.txt", 0};
    if (execute_job(&j2) != 0) return 0;
    // Var olmayan dosya testi - execute_job başarısız olmalı   
    job_t j3 = {3, JOB_FILE_HASH, "tests/non_existent_file.txt", 0};
    if (execute_job(&j3) == 0) return 0;
    
    return 1;
}

int test_invalid_job_type(void)
{
    job_t j = {1, (job_type_t)999, "", 0};
    /* Desteklenmeyen görev türü başarısız olmalı */
    if (execute_job(&j) == 0) return 0;

    /* Null görev başarısız olmalı */
    if (execute_job(NULL) == 0) return 0;

    return 1;
}

int main(void)
{
    printf("Starting Tests...\n");
    
    RUN_TEST(test_job_queue_basic);
    RUN_TEST(test_prime_check);
    RUN_TEST(test_line_count);
    RUN_TEST(test_file_hash);
    RUN_TEST(test_invalid_job_type);
    
    printf("\nTest Summary: %d / %d passed.\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        return 0;
    } else {
        return 1;
    }
}
