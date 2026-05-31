# Sistem Tasarımı ve Mimarisi

## Genel Bakış

**Paralel Görev İşleyici**, POSIX pthread API'sini kullanan multi-threaded bir C uygulamasıdır. Sistem, ortak bir FIFO kuyruğundan görevleri alıp, belirlenen sayıda worker thread tarafından paralel olarak işlenmesini sağlar.

## Sistem Mimarisi

```
┌─────────────────────────────────────────────────────────────┐
│                   ANA THREAD (main.c)                        │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 1. CLI argümanları parse et                         │   │
│  │ 2. Dosyadan görevleri yükle                         │   │
│  │ 3. Thread pool oluştur                              │   │
│  │ 4. İşleri kuyruğa gönder (producer)                 │   │
│  │ 5. Thread pool shutdown başlat                      │   │
│  │ 6. Metrikleri yazdır                                │   │
│  └─────────────────────────────────────────────────────┘   │
└──────────────────────────┬──────────────────────────────────┘
                           │
                ┌──────────▼──────────┐
                │  İŞ KUYRUĞU (FIFO)  │
                │  (job_queue.c)      │
                │ ┌────────────────┐  │
                │ │ Mutex Protected │  │
                │ │ Circular Buffer │  │
                │ └────────────────┘  │
                └──────┬─────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │  (N threads)
     ┌──▼──┐       ┌──▼──┐       ┌──▼──┐
     │ W-0 │       │ W-1 │       │ W-N │
     │     │       │     │       │     │
     └─────┘       └─────┘       └─────┘
     (Consumer threads - tasks.c)
```

## Modüller Detayı

### 1. **main.c** - Ana Program
**Sorumluluklar:**
- Command-line argümanları parse etme
- Dosyadan görevleri yükleme
- Thread pool yönetimi (create/shutdown/destroy)
- Görevleri kuyruğa gönderme (producer role)
- SIGINT (Ctrl+C) sinyali işleme
- Hata yönetimi ve raporlama

**Akış:**
```
start
  ├─ Parse CLI arguments
  ├─ Load jobs from file
  ├─ Create thread pool
  ├─ Submit jobs to queue
  ├─ Wait for shutdown
  ├─ Print metrics
  └─ Cleanup
end
```

### 2. **thread_pool.c** - Thread Pool Yönetimi
**Veri Yapısı:**
```c
struct thread_pool {
    pthread_t *workers;              // Worker thread array
    int worker_count;                // Kaç thread oluşturuldu
    int is_shutdown;                 // Shutdown bayrağı
    job_queue_t *queue;              // Ortak iş kuyruğu
    pthread_mutex_t shutdown_mutex;  // Senkronizasyon
    pthread_cond_t queue_not_empty;  // Bekleme sinyali
};
```

**Fonksiyonlar:**

| Fonksiyon | Amaç |
|-----------|------|
| `thread_pool_create()` | Worker thread'ler oluştur, mutex/cond_var başlat |
| `thread_pool_submit()` | İşi kuyruğa gönder ve signal gönder |
| `thread_pool_shutdown()` | Broadcast signal ve pthread_join() |
| `thread_pool_destroy()` | Kaynakları temizle |
| `worker_thread_function()` | Worker'ın sonsuz döngüsü |

**Worker Thread Yaşam Döngüsü:**
```
while true:
    ├─ Lock shutdown_mutex
    ├─ While queue empty AND NOT shutdown:
    │  └─ pthread_cond_wait()
    ├─ Unlock mutex
    ├─
    ├─ If shutdown AND queue empty: break
    ├─ Pop from queue
    ├─ Execute job
    ├─ Record metrics
    └─ Repeat
```

### 3. **job_queue.c** - FIFO Görev Kuyruğu
**Veri Yapısı (Circular Buffer):**
```c
struct job_queue {
    job_t *items;              // Job array
    int capacity;              // Maksimum kapasite
    int size;                  // Şu anki eleman sayısı
    int front;                 // Başlangıç indeksi
    int rear;                  // Bitiş indeksi
    pthread_mutex_t mutex;     // Senkronizasyon
};
```

**Operasyonlar:**
- `push(job)`: Kuyruğun sonuna iş ekle (mutex korumalı)
- `pop(job*)`: Kuyruğun başından iş al (mutex korumalı)
- `is_empty()`: Kuyruk boş mu?
- `is_full()`: Kuyruk dolu mu?

**FIFO Mantığı:**
```
Initial: front=0, rear=0, size=0

Push Job-1:
  items[0] = Job-1
  rear = (0+1) % capacity = 1
  size = 1
  
Push Job-2:
  items[1] = Job-2
  rear = (1+1) % capacity = 2
  size = 2
  
Pop:
  return items[0] (Job-1)
  front = (0+1) % capacity = 1
  size = 1
  
Pop:
  return items[1] (Job-2)
  front = (1+1) % capacity = 2
  size = 0 ← EMPTY
```

### 4. **job.c** - İş Modeli Yardımcıları
```c
typedef enum {
    JOB_FILE_HASH = 1,
    JOB_LINE_COUNT = 2,
    JOB_PRIME_CHECK = 3
} job_type_t;

typedef struct {
    int id;
    job_type_t type;
    char path[256];      // Dosya yolu
    long number;         // Sayısal parametre
} job_t;
```

### 5. **tasks.c** - Görev Uygulamaları
**Desteklenen Görevler:**

| Görev | Parametre | Açıklama |
|-------|-----------|----------|
| `PRIME_CHECK` | `number` | Trial division ile asal mı kontrol |
| `LINE_COUNT` | `path` | Dosyadaki `\n` karakterlerini say |
| `FILE_HASH` | `path` | Basit XOR-based file hash |

**Örnek: Prime Check**
```c
static int is_prime(long number) {
    if (number < 2) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    
    for (long d = 3; d * d <= number; d += 2)
        if (number % d == 0) return 0;
    
    return 1;
}
```

### 6. **logger.c** - Loglama Sistemi
**Seviyeleri:**
- `logger_info()` - Yeşil renk, bilgi mesajları
- `logger_error()` - Kırmızı renk, hata mesajları
- `logger_debug()` - Sarı renk, debug mesajları

**Çıktı Formatı:**
```
[HH:MM:SS] [LEVEL] | Message text
```

### 7. **metrics.c** - Performans Metriği
**Toplanan Veriler:**
- Total jobs
- Successful jobs
- Failed jobs
- Total time spent
- Average job time
- Throughput (jobs/second)

**Rapor Formatı:**

    ┌──────────────────────────────────────────────────────┐
    │              📊 PERFORMANS RAPORU 📊               │
    ├──────────────────────────────────────────────────────┤
    │ Worker Sayısı:              4                     │
    │ Toplam İş:                  15                    │
    │ ✓ Başarılı:                 15                    │
    │ ✗ Başarısız:                0                     │
    │ Başarı Oranı:               100%                   │
    │ Toplam İş Süresi (CPU):     0.0162 saniye           │
    │ Ortalama Süre:              0.0011 saniye           │
    │ İş/Saniye (1/Ort.):         926.34 İş/s             │
    └──────────────────────────────────────────────────────┘

## Senkronizasyon Mekanizması

### Mutex Kullanım Alanları

1. **İş Kuyruğu (job_queue)**
   - Push/pop atomik yapma
   - Race condition önleme

2. **Shutdown Durumu (shutdown_mutex)**
   - is_shutdown bayrağı güvenli okuma
   - Condition variable ile bağlantı

### Condition Variable

**queue_not_empty:**
```
Worker Thread:
  ├─ Lock mutex
  ├─ While queue.is_empty() && !shutdown:
  │  └─ pthread_cond_wait(&queue_not_empty, &mutex)
  │     (Burada mutex unlock olur, thread uyur)
  ├─ Unlock mutex
  └─ ...

Main Thread:
  ├─ Lock mutex
  ├─ Push job to queue
  ├─ pthread_cond_signal(&queue_not_empty)
  │  (Bir worker'ı uyandır)
  └─ Unlock mutex
```

### Graceful Shutdown Mantığı

```
User presses Ctrl+C:
  ├─ signal_handler() çağrılır
  └─ g_interrupted = 1

Main Thread:
  ├─ thread_pool_shutdown() çağırır
  │  ├─ is_shutdown = 1
  │  ├─ pthread_cond_broadcast(&queue_not_empty)
  │  │  (Tüm worker'ları uyandır)
  │  ├─ For each worker:
  │  │  └─ pthread_join(worker_id, NULL)
  │  │     (Worker bitene kadar bekle)
  │  └─ return
  │
  └─ Kaynakları temizle

Worker Thread:
  ├─ Uyandırıldıktan sonra loop devam eder
  ├─ is_shutdown check eder
  ├─ If shutdown AND queue empty:
  │  └─ break (thread çıkış yap)
  └─ Aksi durumda kalan işleri işle
```

## Veri Akışı

### İş İşleme Akışı

```
1. Main: Load jobs
        ├─ Parse command line
        ├─ Load from file
        └─ jobs[] array oluştur

2. Main: Submit Jobs
        ├─ Lock queue.mutex
        ├─ Push job[i] to queue
        ├─ Unlock queue.mutex
        └─ Signal worker thread

3. Worker: Process Job
        ├─ Lock queue.mutex
        ├─ Check queue not empty
        ├─ Unlock queue.mutex
        ├─ Pop job
        ├─ Execute job (execute_job)
        ├─ Record metrics
        └─ Repeat

4. Main: Shutdown
        ├─ Signal all workers
        ├─ Wait for all threads (pthread_join)
        ├─ Print report
        └─ Exit
```

## Performans Özellikleri

### İş Dağılımı Stratejisi
- **FIFO düzen**: Görevler sırasıyla işlenir
- **Load balancing**: Worker'lar kuyruğu paylaşır
- **No job stealing**: Her worker sadece kuyruğu dinler

### Optimizasyon Noktaları

| Senaryo | Önerilen Config |
|---------|-----------------|
| I/O-bound (dosya, network) | Birçok thread (8-16) |
| CPU-bound (prime, hash) | Az thread (2-4) |
| Mixed | CPU core sayısı kadar |

### Verimlilik Ölçütleri
```
Throughput = Total_Jobs / Total_Time (jobs/sec)
Utilization = Actual_Work_Time / Wall_Clock_Time (%)
Fairness = Min_Job_Time / Max_Job_Time (0-1)
```

## Hata Yönetimi Stratejisi

```
CLI Parse Errors:
  └─ Print help, exit with code 1

File Loading Errors:
  ├─ File not found → logger_error()
  ├─ Invalid format → Skip line, continue
  └─ No valid jobs → Exit with code 1

Thread Creation Errors:
  ├─ pthread_create() fail → Cleanup partial pool
  └─ mutex/cond_var init fail → Free and exit

Runtime Errors:
  ├─ Job execution fail → Log and continue
  ├─ Queue full → logger_error()
  └─ Memory allocation → Cleanup and exit
```

## Gelecek İyileştirmeler

### Kısa Vadeli
1. **Job Prioritization**: Bazı işleri öncelikle işle
2. **Batch Processing**: Birçok işi bir kere gönder
3. **Job Dependencies**: İş-İş bağımlılıkları

### Orta Vadeli
4. **Work Stealing**: İşsiz worker'lar işi çaları
5. **Dynamic Thread Pool**: Worker sayısını dinamik ayarla
6. **Statistics per Worker**: Her worker'ın kendi metrikleri

### Uzun Vadeli
7. **Network Distribution**: Uzaktan iş gönderme
8. **Persistence**: Job database (SQLite, PostgreSQL)
9. **Monitoring**: Real-time dashboard
10. **Retry Mechanism**: Başarısız işleri yeniden dene

---

**Belge Sürümü**: 2.0  
**Son Güncelleme**: 31 Mayıs 2026  
**Durum**: Production Ready ✅
