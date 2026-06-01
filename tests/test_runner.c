#include "job.h"
#include "job_queue.h"
#include "tasks.h"
#include "logger.h"
#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int test_job_queue_edges(void)
{
    job_t j = {1, JOB_PRIME_CHECK, "", 7};
    job_t out;
    job_queue_t *q;

    if (job_queue_create(0) != NULL) return 0;
    if (job_queue_create(-4) != NULL) return 0;

    job_queue_destroy(NULL);

    if (!job_queue_is_empty(NULL)) return 0;
    if (job_queue_is_full(NULL)) return 0;
    if (job_queue_size(NULL) != 0) return 0;
    if (job_queue_capacity(NULL) != 0) return 0;
    if (job_queue_push(NULL, j) == 0) return 0;
    if (job_queue_pop(NULL, &out) == 0) return 0;

    q = job_queue_create(3);
    if (!q) return 0;

    if (job_queue_capacity(q) != 3) return 0;
    if (job_queue_size(q) != 0) return 0;
    if (job_queue_pop(q, NULL) == 0) return 0;

    if (job_queue_push(q, j) != 0) return 0;
    if (job_queue_size(q) != 1) return 0;

    if (job_queue_pop(q, &out) != 0) return 0;
    if (out.id != j.id) return 0;
    if (job_queue_size(q) != 0) return 0;

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
    const char *line_file = "tests/test_line_count_tmp.txt";
    const char *empty_file = "tests/test_empty_tmp.txt";
    FILE *f = fopen(line_file, "w");
    if (f) {
        fprintf(f, "line1\nline2\nline3\n");
        fclose(f);
    }

    job_t j1 = {1, JOB_LINE_COUNT, "tests/test_line_count_tmp.txt", 0};
    if (execute_job(&j1) != 0) return 0;

    // Boş dosya testi
    FILE *f_empty = fopen(empty_file, "w");
    if (f_empty) {
        fclose(f_empty);
    }
    job_t j2 = {2, JOB_LINE_COUNT, "tests/test_empty_tmp.txt", 0};
    if (execute_job(&j2) != 0) return 0;

    // Var olmayan dosya testi - execute_job başarısız olmalı
    job_t j3 = {3, JOB_LINE_COUNT, "tests/non_existent_file.txt", 0};
    if (execute_job(&j3) == 0) return 0;

    remove(line_file);
    remove(empty_file);
    return 1;
}

int test_file_hash(void)
{
    job_t j1 = {1, JOB_FILE_HASH, "tests/sample1.txt", 0};
    if (execute_job(&j1) != 0) return 0;

    // Boş dosya testi
    const char *empty_file = "tests/test_empty_hash_tmp.txt";
    FILE *f_empty = fopen(empty_file, "w");
    if (f_empty) {
        fclose(f_empty);
    }
    job_t j2 = {2, JOB_FILE_HASH, "tests/test_empty_hash_tmp.txt", 0};
    if (execute_job(&j2) != 0) return 0;
    // Var olmayan dosya testi - execute_job başarısız olmalı
    job_t j3 = {3, JOB_FILE_HASH, "tests/non_existent_file.txt", 0};
    if (execute_job(&j3) == 0) return 0;

    remove(empty_file);
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

int test_thread_pool_edges(void)
{
    thread_pool_t *pool;
    job_t valid_job = {1, JOB_PRIME_CHECK, "", 17};
    job_t invalid_job = {2, (job_type_t)999, "", 0};

    if (thread_pool_create(0, 2) != NULL) return 0;
    if (thread_pool_create(2, 0) != NULL) return 0;
    if (thread_pool_submit(NULL, valid_job) == 0) return 0;

    thread_pool_shutdown(NULL);
    thread_pool_destroy(NULL);

    pool = thread_pool_create(1, 2);
    if (!pool) return 0;
    if (thread_pool_submit(pool, valid_job) != 0) return 0;
    if (thread_pool_submit(pool, invalid_job) != 0) return 0;
    thread_pool_shutdown(pool);

    if (thread_pool_submit(pool, valid_job) == 0) return 0;

    thread_pool_destroy(pool);
    return 1;
}

int main(void)
{
    printf("Starting Tests...\n");

    RUN_TEST(test_job_queue_basic);
    RUN_TEST(test_job_queue_edges);
    RUN_TEST(test_prime_check);
    RUN_TEST(test_line_count);
    RUN_TEST(test_file_hash);
    RUN_TEST(test_invalid_job_type);
    RUN_TEST(test_thread_pool_edges);

    printf("\nTest Summary: %d / %d passed.\n", tests_passed, tests_run);

    if (tests_passed == tests_run) {
        return 0;
    } else {
        return 1;
    }
}
