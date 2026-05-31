# Paralel Görev İşleyici 🚀

**Thread Pool Tabanlı Paralel Görev İşleyici** - C dilinde POSIX/Linux API kullanarak geliştirilmiş, performans odaklı bir sistem.

## 📋 Özellikler

- ✅ **Çoklu Worker Thread'ler**: Yapılandırılabilir sayıda worker thread
- ✅ **FIFO İş Kuyruğu**: Pthread mutex ile korunan ortak kuyruk
- ✅ **Condition Variable**: Efficient bekleme/uyandırma mekanizması
- ✅ **Graceful Shutdown**: Kontrollü kapanış ve kaynak temizliği
- ✅ **3 İş Türü**: 
  - `PRIME_CHECK`: Asal sayı kontrolü
  - `LINE_COUNT`: Dosyadaki satır sayımı
  - `FILE_HASH`: Basit dosya hash'i
- ✅ **Renkli Terminal Çıktısı**: ANSI renk desteği
- ✅ **Gelişmiş Performans Raporu**: Throughput, başarı oranı, zaman ölçümü
- ✅ **CLI Argümanları**: `--threads`, `--queue-size`, `--input`
- ✅ **Hata Yönetimi**: Dosya hatası, geçersiz parametre, SIGINT (Ctrl+C) işleme

## 🛠️ Teknik Detaylar

### Sistem Mimarisi

```
┌─────────────────────────────────────────────────┐
│            ANA THREAD (main)                     │
│  - CLI argümanları parse                         │
│  - Dosyadan işleri yükle                         │
│  - Thread pool başlat                            │
│  - İşleri kuyruğa gönder                         │
└─────────────────────────────────────────────────┘
                      ↓
        ┌─────────────────────────────────┐
        │   ORTAK İŞ KUYRUĞU (FIFO)       │
        │   Mutex ile korunmuş             │
        └─────────────────────────────────┘
                      ↓
   ┌──────────┬──────────┬──────────┬──────────┐
   ↓          ↓          ↓          ↓          ↓
[Worker-0] [Worker-1] [Worker-2] [Worker-3] ... [Worker-N]
   │          │          │          │          │
   └──────────┴──────────┴──────────┴──────────┘
                      ↓
        ┌─────────────────────────────────┐
        │    PERFORMANS METRİKLERİ        │
        │  - İş süresi                    │
        │  - Başarı/Başarısızlık          │
        │  - Throughput (İş/saniye)       │
        └─────────────────────────────────┘
```

### Veri Yapıları

**Job (İş) Modeli:**
```c
typedef struct {
    int id;              // İş kimliği
    job_type_t type;     // PRIME_CHECK, LINE_COUNT, FILE_HASH
    char path[256];      // Dosya yolu (dosya işleri için)
    long number;         // Sayısal parametre (PRIME için)
} job_t;
```

**Thread Pool:**
- `pthread_t *workers` - Worker thread array'ı
- `job_queue_t *queue` - Ortak FIFO kuyruğu
- `pthread_mutex_t shutdown_mutex` - Senkronizasyon
- `pthread_cond_t queue_not_empty` - Bekleme sinyali

**Job Queue (Circular Buffer):**
- Mutex ile korunan circular FIFO
- Push/pop işlemleri atomik
- İş kalmadığında condition variable'de bekleme

## 📦 Kullanım

### Derleme

```bash
# Proje klasörüne gidin
cd thread-pool-gorev-processor

# Derleyin
make

# Temizle
make clean
```

### Çalıştırma

#### Default Yapılandırma (4 worker)
```bash
./threadpool_app
```

#### Custom Yapılandırma
```bash
./threadpool_app --threads 8 --queue-size 64 --input tasks.txt
```

#### Yardım
```bash
./threadpool_app --help
```

### Komut Satırı Seçenekleri

| Seçenek | Açıklama | Varsayılan |
|---------|----------|-----------|
| `--threads <N>` | Worker thread sayısı (1-64) | 4 |
| `--queue-size <N>` | İş kuyruğu kapasitesi (1-4096) | 32 |
| `--input <dosya>` | Görevleri okunan dosya | Örnek görevler |
| `--help, -h` | Yardımı göster | - |

## 📝 Girdi Dosyası Formatı

Dosya satır satır görevleri içerir. Format: `GÖREV_TİPİ PARAMETRE`

**Örnek (jobs_mixed.txt):**
```text
# Asal sayı kontrolü
PRIME 17
PRIME 999983

# Dosya satır sayımı
LINE_COUNT tests/sample1.txt

# Dosya hash'eme
FILE_HASH tests/sample2.txt

# Daha fazla işler
PRIME 1234567
```

**Destek Gören Görev Tipleri:**
- `PRIME <sayı>` - Asal sayı kontrolü
- `LINE_COUNT <dosya>` - Dosyadaki satır sayısı
- `FILE_HASH <dosya>` - Dosya hash değeri

**Notlar:**
- Boş satırlar otomatikman atlanır
- `#` ile başlayan satırlar yorum olarak işlenir
- Geçersiz satırlar hata loglanır ve atlanır
- Maksimum 1024 görev desteklenebilir

## 💡 Örnek Çalıştırmalar

### Test 1: Default Örnek Görevler
```bash
$ ./threadpool_app
```

**Çıktı:**
```
╔═══════════════════════════════════════════════════╗
║         🚀 PARALEL GÖREV İŞLEYİCİ 🚀            ║
║      Thread Pool Tabanlı Görev Processor          ║
╚═══════════════════════════════════════════════════╝

[11:36:17] INFO     | Yapılandırma: 4 worker, kapasite 32
[11:36:17] INFO     | Örnek görevler kullanılıyor
[11:36:17] INFO     | Thread pool oluşturuluyor...
...
[11:36:17] INFO     | 4 görev kuyruğa ekleniyor...
[11:36:17] INFO     | ✓ Program başarılı şekilde tamamlandı

┌──────────────────────────────────────────────────────┐
│              📊 PERFORMANS RAPORU 📊               │
├──────────────────────────────────────────────────────┤
│ Worker Sayısı:              4                     │
│ Toplam İş:                  4                     │
│ ✓ Başarılı:                 4                     │
│ ✗ Başarısız:                0                     │
│ Başarı Oranı:               100%                   │
│ Toplam İş Süresi (CPU):     0.0052 saniye           │
│ Ortalama Süre:              0.0013 saniye           │
│ İş/Saniye (1/Ort.):         762.38 İş/s             │
└──────────────────────────────────────────────────────┘
```

### Test 2: Custom Yapılandırma
```bash
$ ./threadpool_app --threads 2 --queue-size 16 --input tests/jobs_mixed.txt
```

**Sonuç:** 7 görev, 2 worker ile 0.0116 saniyede tamamlandi

### Test 3: Stress Test
```bash
$ ./threadpool_app --threads 4 --queue-size 32 --input tests/jobs_stress.txt
```

**Sonuç:** 15 görev, 4 worker ile **926 İş/saniye** throughput!

## 🔧 Proje Yapısı

```
thread-pool-gorev-processor/
├── include/              # Header dosyaları
│   ├── job.h            # Job veri modeli
│   ├── job_queue.h      # FIFO kuyruğu
│   ├── thread_pool.h    # Thread pool arayüzü
│   ├── tasks.h          # Görev işleyici
│   ├── logger.h         # Loglama
│   └── metrics.h        # Performans metriği
├── src/                  # Kaynak dosyaları
│   ├── main.c           # Ana program
│   ├── thread_pool.c    # Thread pool implementasyonu
│   ├── job_queue.c      # FIFO implementasyonu
│   ├── job.c            # Job yardımcıları
│   ├── tasks.c          # Görev işleyiciler (PRIME, LINE_COUNT, FILE_HASH)
│   ├── logger.c         # Loglama implementasyonu
│   └── metrics.c        # Metrik toplama
├── tests/               # Test dosyaları
│   ├── jobs_small.txt
│   ├── jobs_mixed.txt
│   ├── jobs_stress.txt
│   ├── sample1.txt
│   └── sample2.txt
├── Makefile             # Derleme kuralları
└── README.md            # Bu dosya
```

## 🔐 Senkronizasyon Mekanizması

### Mutex Kullanımı
- **İş Kuyruğu**: `pthread_mutex_t` ile push/pop işlemleri korunmuş
- **Shutdown Durumu**: İş işleme sırasında shutdown kontrolü

### Condition Variable
- **queue_not_empty**: Kuyruk boş olduğunda worker'lar uyutulur
- Signal/Broadcast: Yeni iş gelince worker'lar uyandırılır
- Graceful Shutdown: Tüm worker'lara broadcast gönderilir

### Producer-Consumer Modeli
- **Producer** (main thread): İşleri kuyruğa ekle
- **Consumer** (worker threads): İşleri kuyruğu oku ve işle

## 📊 Performans Özellikleri

Tested Configuration:
- **Stress Test**: 15 görev, 4 worker
- **Throughput**: 926 İş/saniye
- **Ortalama İş Süresi**: 1.1 ms
- **Başarı Oranı**: %100

### Optimizasyon İpuçları
```bash
# Birçok I/O işi için daha fazla thread
./threadpool_app --threads 8 --queue-size 64 --input large_file_tasks.txt

# CPU-intensive işler için az thread
./threadpool_app --threads 4 --queue-size 32 --input cpu_tasks.txt
```

## ⚠️ Hata Yönetimi

Program şu hataları ele alır:

- ❌ **Dosya Açılmadı**: `Dosya açılamadı: filename (No such file or directory)`
- ❌ **Geçersiz Dosya Formatı**: `Geçersiz satır formatı (satır N): ...`
- ❌ **Bilinmeyen Görev Tipi**: `Bilinmeyen görev tipi: TYPE`
- ❌ **Geçersiz CLI Argümanı**: `Thread sayısı 1-64 arasında olmalı`
- ⚠️ **SIGINT (Ctrl+C)**: Kalan işleri işlemeyi durdurur ve graceful shutdown başlatır

## 🛠️ Geliştirici Notları

### Eklenebilecek Özellikler
1. İş önceliği (priority queue)
2. Görev bağımlılıkları
3. Uzaktan iş gönderimi (network)
4. Persistent job storage (database)
5. Job retry mekanizması
6. Dinamik worker scaling

### Debug Modu
```bash
# GDB ile debug
gdb ./threadpool_app

# Valgrind ile memory leak kontrolü
valgrind --leak-check=full ./threadpool_app
```

## 📜 Lisans

Bu proje eğitim amaçlı oluşturulmuştur.

## 👨‍💻 Yazarlar

- **Proje Lideri / Entegrasyon**: main.c, Makefile
- **Thread Pool**: thread_pool.c, worker yaşam döngüsü
- **İş Kuyruğu & Model**: job_queue.c, job.c
- **Görevler & Loglama**: tasks.c, logger.c, metrics.c

---

**Son Güncellenme**: 31 Mayıs 2026  
**Versiyon**: 1.0 - Production Ready ✅

Bu dağılım başlangıç önerisidir. Geliştirme sırasında ihtiyaçlara göre görev paylaşımı güncellenebilir.

## Karşılaşılan Problemler

Bu bölüm geliştirme ve test süreci tamamlandıktan sonra doldurulacaktır.
