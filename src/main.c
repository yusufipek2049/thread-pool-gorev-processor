#include "job.h"
#include "logger.h"
#include "metrics.h"
#include "thread_pool.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define DEFAULT_WORKER_COUNT 4
#define DEFAULT_QUEUE_SIZE 32
#define MAX_JOBS 100000

static volatile sig_atomic_t g_interrupted = 0;

thread_pool_t *g_pool = NULL;

typedef struct {
    int worker_count;
    int queue_size;
    char input_file[256];
} options_t;

static void signal_handler(int sig)
{
    (void)sig;
    g_interrupted = 1;
}

static void print_banner(void)
{
    printf("\nParalel Görev İşleyici\n");
    printf("Thread Pool Tabanlı Görev Processor\n\n");
}

static void print_usage(const char *prog_name)
{
    printf("\nKullanım:\n");
    printf("  %s [SEÇENEKLER]\n", prog_name);
    printf("  %s -t 4 -q 32 tests/jobs_mixed.txt\n\n", prog_name);
    printf("Seçenekler:\n");
    printf("  --threads, -t <N>       Worker thread sayısı (varsayılan: %d)\n", DEFAULT_WORKER_COUNT);
    printf("  --queue-size, -q <N>    İş kuyruğu kapasitesi (varsayılan: %d)\n", DEFAULT_QUEUE_SIZE);
    printf("  --input, -i <dosya>     Görevlerin okunacağı dosya\n");
    printf("  --help, -h              Bu yardımı göster\n\n");
}

static int parse_int_range(const char *text, int min_value, int max_value, int *value);
static int parse_positive_long(const char *text, long *value);

static int parse_arguments(int argc, char *argv[], options_t *options)
{
    int i = 1;

    options->worker_count = DEFAULT_WORKER_COUNT;
    options->queue_size = DEFAULT_QUEUE_SIZE;
    memset(options->input_file, 0, sizeof(options->input_file));

    while (i < argc) {
        if (strcmp(argv[i], "--threads") == 0 || strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                logger_error("%s için değer sağlanmalı", argv[i]);
                return -1;
            }
            if (parse_int_range(argv[i + 1], 1, 64, &options->worker_count) != 0) {
                logger_error("Thread sayısı 1-64 arasında olmalı");
                return -1;
            }
            i += 2;
        } else if (strcmp(argv[i], "--queue-size") == 0 || strcmp(argv[i], "-q") == 0) {
            if (i + 1 >= argc) {
                logger_error("%s için değer sağlanmalı", argv[i]);
                return -1;
            }
            if (parse_int_range(argv[i + 1], 1, 4096, &options->queue_size) != 0) {
                logger_error("Kuyruk kapasitesi 1-4096 arasında olmalı");
                return -1;
            }
            i += 2;
        } else if (strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0) {
            if (i + 1 >= argc) {
                logger_error("%s için dosya adı sağlanmalı", argv[i]);
                return -1;
            }
            strncpy(options->input_file, argv[i + 1], sizeof(options->input_file) - 1);
            i += 2;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (argv[i][0] != '-' && options->input_file[0] == '\0') {
            strncpy(options->input_file, argv[i], sizeof(options->input_file) - 1);
            i++;
        } else {
            logger_error("Bilinmeyen seçenek: %s", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

static int parse_int_range(const char *text, int min_value, int max_value, int *value)
{
    char *endptr;
    long parsed;

    errno = 0;
    parsed = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0' ||
        parsed < min_value || parsed > max_value) {
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

static int parse_positive_long(const char *text, long *value)
{
    char *endptr;
    long parsed;

    errno = 0;
    parsed = strtol(text, &endptr, 10);
    if (errno != 0 || endptr == text || *endptr != '\0' || parsed <= 0) {
        return -1;
    }

    *value = parsed;
    return 0;
}

static int load_jobs_from_file(const char *filename, job_t *jobs, int max_jobs, int *invalid_count)
{
    FILE *file;
    int job_count = 0;
    char line[512];
    char job_type_str[32];
    char param[256];
    int line_num = 0;

    if (invalid_count != NULL) {
        *invalid_count = 0;
    }

    if (filename[0] == '\0') {
        logger_info("Örnek görevler kullanılıyor");
        jobs[0] = (job_t){1, JOB_PRIME_CHECK, "", 999983};
        jobs[1] = (job_t){2, JOB_LINE_COUNT, "tests/sample1.txt", 0};
        jobs[2] = (job_t){3, JOB_FILE_HASH, "tests/sample2.txt", 0};
        jobs[3] = (job_t){4, JOB_PRIME_CHECK, "", 1234567};
        return 4;
    }

    file = fopen(filename, "r");
    if (file == NULL) {
        logger_error("Dosya açılamadı: %s (%s)", filename, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), file) != NULL && job_count < max_jobs) {
        line_num++;

        /* Boş satırları ve yorumları atla */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        if (sscanf(line, "%31s %255s", job_type_str, param) != 2) {
            logger_error("Geçersiz satır formatı (satır %d): %s", line_num, line);
            if (invalid_count != NULL) {
                (*invalid_count)++;
            }
            continue;
        }

        if (job_count >= max_jobs) {
            logger_error("Maksimum görev sayısına ulaşıldı (%d)", max_jobs);
            break;
        }

        job_t new_job;
        memset(&new_job, 0, sizeof(new_job));
        new_job.id = job_count + 1;

        if (strcmp(job_type_str, "PRIME") == 0) {
            new_job.type = JOB_PRIME_CHECK;
            if (parse_positive_long(param, &new_job.number) != 0) {
                logger_error("Geçersiz PRIME parametresi (satır %d): %s", line_num, param);
                if (invalid_count != NULL) {
                    (*invalid_count)++;
                }
                continue;
            }
        } else if (strcmp(job_type_str, "LINE_COUNT") == 0) {
            new_job.type = JOB_LINE_COUNT;
            strncpy(new_job.path, param, sizeof(new_job.path) - 1);
            new_job.number = 0;
        } else if (strcmp(job_type_str, "FILE_HASH") == 0) {
            new_job.type = JOB_FILE_HASH;
            strncpy(new_job.path, param, sizeof(new_job.path) - 1);
            new_job.number = 0;
        } else {
            logger_error("Bilinmeyen görev tipi (satır %d): %s", line_num, job_type_str);
            if (invalid_count != NULL) {
                (*invalid_count)++;
            }
            continue;
        }

        jobs[job_count++] = new_job;
    }

    if (ferror(file)) {
        logger_error("Dosya okuma hatası: %s", strerror(errno));
        fclose(file);
        return -1;
    }

    fclose(file);

    if (job_count == 0 && (invalid_count == NULL || *invalid_count == 0)) {
        logger_error("Dosyada geçerli görev bulunamadı: %s", filename);
        return -1;
    }

    logger_info("Dosyadan %d görev yüklendi: %s", job_count, filename);
    return job_count;
}

int main(int argc, char *argv[])
{
    options_t options;
    thread_pool_t *pool = NULL;
    job_t *jobs = NULL;
    int job_count;
    int invalid_job_count = 0;
    int i;
    int result;
    int exit_code = 0;

    /* Sinyal yöneticisini kur */
    signal(SIGINT, signal_handler);

    print_banner();

    result = parse_arguments(argc, argv, &options);
    if (result != 0) {
        return result == 1 ? 0 : 1;
    }

    jobs = malloc(MAX_JOBS * sizeof(job_t));
    if (jobs == NULL) {
        logger_error("Bellek ayrılamadı (görevler için)");
        return 1;
    }

    logger_info("Yapılandırma: %d worker, kapasite %d",
                options.worker_count, options.queue_size);

    job_count = load_jobs_from_file(options.input_file, jobs, MAX_JOBS, &invalid_job_count);
    if (job_count < 0) {
        logger_error("Görev yüklenemedi");
        exit_code = 1;
        goto cleanup;
    }

    logger_info("Thread pool oluşturuluyor...");
    pool = thread_pool_create(options.worker_count, options.queue_size);
    if (pool == NULL) {
        logger_error("Thread pool oluşturulamadı");
        exit_code = 1;
        goto cleanup;
    }

    g_pool = pool;

    for (i = 0; i < invalid_job_count; i++) {
        metrics_record_job_failure(0.0);
    }

    if (invalid_job_count > 0) {
        logger_error("%d geçersiz görev başarısız olarak kaydedildi", invalid_job_count);
    }

    logger_info("----------------------------------------");
    logger_info("%d görev kuyruğa ekleniyor...", job_count);
    logger_info("----------------------------------------");

    for (i = 0; i < job_count; i++) {
        if (g_interrupted) {
            logger_info("Kullanıcı tarafından durduruldu. Kalan görevler işlenmeyecek.");
            break;
        }

        if (thread_pool_submit(pool, jobs[i]) != 0) {
            logger_error("Görev %d gönderilemedi", i + 1);
            metrics_record_job_failure(0.0);
            exit_code = 1;
        }
    }

    logger_info("----------------------------------------");
    logger_info("Tüm görevler gönderildi. Kapanış bekleniyor...");
    logger_info("----------------------------------------");

    thread_pool_shutdown(pool);
    thread_pool_destroy(pool);
    pool = NULL;
    g_pool = NULL;

    if (g_interrupted) {
        printf("\nProgram kullanıcı tarafından durduruldu\n");
        exit_code = 130; /* SIGINT exit code */
    } else {
        logger_info("Program başarılı şekilde tamamlandı");
    }

cleanup:
    if (pool != NULL) {
        thread_pool_shutdown(pool);
        thread_pool_destroy(pool);
        g_pool = NULL;
    }

    free(jobs);
    return exit_code;
}
