#include "job.h"
#include "logger.h"
#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BOLD    "\x1b[1m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DEFAULT_WORKER_COUNT 4
#define DEFAULT_QUEUE_SIZE 32
#define MAX_JOBS 1024

static volatile sig_atomic_t g_interrupted = 0;

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
    printf("\n%s%s", ANSI_COLOR_BOLD, ANSI_COLOR_CYAN);
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║                                                   ║\n");
    printf("║         🚀 PARALEL GÖREV İŞLEYİCİ 🚀            ║\n");
    printf("║      Thread Pool Tabanlı Görev Processor          ║\n");
    printf("║                                                   ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("%s\n", ANSI_COLOR_RESET);
}

static void print_usage(const char *prog_name)
{
    printf("\n%sKullanım:%s\n", ANSI_COLOR_BOLD, ANSI_COLOR_RESET);
    printf("  %s [SEÇENEKLER]\n\n", prog_name);
    printf("Seçenekler:\n");
    printf("  --threads <N>       Worker thread sayısı (varsayılan: %d)\n", DEFAULT_WORKER_COUNT);
    printf("  --queue-size <N>    İş kuyruğu kapasitesi (varsayılan: %d)\n", DEFAULT_QUEUE_SIZE);
    printf("  --input <dosya>     Görevleri okunan dosya\n");
    printf("  --help              Bu yardımı göster\n\n");
}

static int parse_arguments(int argc, char *argv[], options_t *options)
{
    int i = 1;

    options->worker_count = DEFAULT_WORKER_COUNT;
    options->queue_size = DEFAULT_QUEUE_SIZE;
    memset(options->input_file, 0, sizeof(options->input_file));

    while (i < argc) {
        if (strcmp(argv[i], "--threads") == 0) {
            if (i + 1 >= argc) {
                logger_error("--threads için değer sağlanmalı");
                return -1;
            }
            options->worker_count = atoi(argv[i + 1]);
            if (options->worker_count <= 0 || options->worker_count > 64) {
                logger_error("Thread sayısı 1-64 arasında olmalı");
                return -1;
            }
            i += 2;
        } else if (strcmp(argv[i], "--queue-size") == 0) {
            if (i + 1 >= argc) {
                logger_error("--queue-size için değer sağlanmalı");
                return -1;
            }
            options->queue_size = atoi(argv[i + 1]);
            if (options->queue_size <= 0 || options->queue_size > 4096) {
                logger_error("Kuyruk kapasitesi 1-4096 arasında olmalı");
                return -1;
            }
            i += 2;
        } else if (strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                logger_error("--input için dosya adı sağlanmalı");
                return -1;
            }
            strncpy(options->input_file, argv[i + 1], sizeof(options->input_file) - 1);
            i += 2;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 1;
        } else {
            logger_error("Bilinmeyen seçenek: %s", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }

    return 0;
}

static int load_jobs_from_file(const char *filename, job_t *jobs, int max_jobs)
{
    FILE *file;
    int job_count = 0;
    char line[512];
    char job_type_str[32];
    char param[256];
    int line_num = 0;

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
            continue;
        }

        if (job_count >= max_jobs) {
            logger_error("Maksimum görev sayısına ulaşıldı (%d)", max_jobs);
            break;
        }

        job_t new_job;
        new_job.id = job_count + 1;

        if (strcmp(job_type_str, "PRIME") == 0) {
            new_job.type = JOB_PRIME_CHECK;
            new_job.number = atol(param);
            memset(new_job.path, 0, sizeof(new_job.path));
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
    
    if (job_count == 0) {
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
    job_t jobs[MAX_JOBS];
    int job_count;
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

    logger_info("Yapılandırma: %d worker, kapasite %d", 
                options.worker_count, options.queue_size);

    job_count = load_jobs_from_file(options.input_file, jobs, MAX_JOBS);
    if (job_count <= 0) {
        logger_error("Görev yüklenemedi");
        return 1;
    }

    logger_info("Thread pool oluşturuluyor...");
    pool = thread_pool_create(options.worker_count, options.queue_size);
    if (pool == NULL) {
        logger_error("Thread pool oluşturulamadı");
        return 1;
    }

    g_pool = pool;

    logger_info("═══════════════════════════════════════════════════");
    logger_info("%d görev kuyruğa ekleniyor...", job_count);
    logger_info("═══════════════════════════════════════════════════");

    for (i = 0; i < job_count; i++) {
        if (g_interrupted) {
            logger_info("Kullanıcı tarafından durduruldu. Kalan görevler işlenmeyecek.");
            break;
        }
        
        if (thread_pool_submit(pool, jobs[i]) != 0) {
            logger_error("Görev %d gönderilemedi", i + 1);
            exit_code = 1;
        }
    }

    logger_info("═══════════════════════════════════════════════════");
    logger_info("Tüm görevler gönderildi. Kapanış bekleniyor...");
    logger_info("═══════════════════════════════════════════════════");

    thread_pool_shutdown(pool);
    thread_pool_destroy(pool);
    g_pool = NULL;

    if (g_interrupted) {
        printf("\n%s⚠️  Program kullanıcı tarafından durduruldu%s\n", 
               ANSI_COLOR_CYAN, ANSI_COLOR_RESET);
        exit_code = 130; /* SIGINT exit code */
    } else {
        logger_info("%s✓ Program başarılı şekilde tamamlandı%s",
                    ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    }

    return exit_code;
}
