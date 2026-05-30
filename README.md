# Paralel Görev İşleyici

Bu depo, Thread Pool Tabanlı Paralel Görev İşleyici projesi için başlangıç iskeletidir. Kodlar henüz final uygulama olarak düşünülmemelidir; amaç ekip üyelerinin aynı klasör yapısı, aynı header imzaları ve aynı çalışma düzeni üzerinden geliştirmeye başlamasıdır.

## Amaç

Projenin amacı, ortak bir iş kuyruğuna eklenen görevleri belirli sayıda worker thread ile paralel olarak işleyen bir C uygulaması geliştirmektir. Uygulama POSIX/Linux ortamında çalışacak ve `pthread`, mutex, condition variable, dosya işlemleri, hata yönetimi ve performans ölçümü gibi sistem programlama konularını bir araya getirecektir.

## Tasarım

Sistem ana thread, ortak iş kuyruğu ve worker thread havuzundan oluşur. Ana thread girdi dosyasındaki işleri okuyup kuyruğa ekler. Worker thread'ler kuyruktan görev alır, iş tipine göre ilgili fonksiyonu çalıştırır ve sonucu loglar.

Bu ilk iskelette arayüzler sabitlenmiş, modüller ayrılmış ve proje derlenebilir hale getirilmiştir. Gerçek paralel çalışma, condition variable ile bekleme/uyandırma ve ayrıntılı metrikler sonraki geliştirme adımlarında tamamlanacaktır.

## Kullanılan Sistem Programlama Kavramları

- `pthread` ile thread yönetimi
- Mutex ile ortak veriyi koruma
- Condition variable veya semaphore ile bekleme/uyandırma
- Producer-consumer modeli
- Dosya okuma ve basit dosya işleme
- `clock_gettime()` ile süre ölçümü
- Hata kontrolü ve loglama
- Kontrollü kapanış mantığı

## Kurulum

Linux veya POSIX uyumlu bir ortamda `gcc`, `make` ve pthread desteği yeterlidir.

```bash
make
```

Derleme sonucunda proje kökünde `threadpool_app` çalıştırılabilir dosyası oluşur.

## Çalıştırma Adımları

Örnek çalıştırma:

```bash
make run
```

Doğrudan çalıştırma:

```bash
./threadpool_app
```

İlerleyen aşamada programın şu formata uygun argümanları desteklemesi planlanmaktadır:

```bash
./threadpool_app --threads 4 --queue-size 32 --input tests/jobs_mixed.txt
```

## Girdi Dosyası Formatı

Girdi dosyasında her satır bir işi temsil eder. Şimdilik planlanan format şöyledir:

```text
<IS_TIPI> <PARAMETRE>
```

Örnek:

```text
PRIME 999983
LINE_COUNT tests/sample1.txt
FILE_HASH tests/sample2.txt
PRIME 1234567
```

Boş satırlar ve hatalı satırlar ileride güvenli biçimde ele alınacaktır. Hatalı bir işin tüm programı durdurmaması hedeflenmektedir.

## Desteklenen İş Tipleri

| İş Tipi | Açıklama |
|---|---|
| `PRIME` | Verilen sayının asal olup olmadığını kontrol eder. |
| `LINE_COUNT` | Verilen metin dosyasındaki satır sayısını hesaplar. |
| `FILE_HASH` | Verilen dosya için basit bir hash değeri üretir. |

Kod tarafında bu işler `JOB_PRIME_CHECK`, `JOB_LINE_COUNT` ve `JOB_FILE_HASH` enum değerleriyle temsil edilir.

## Testler

Bu bölüm geliştirme ve test süreci tamamlandıktan sonra doldurulacaktır.

## Performans Değerlendirmesi

Bu bölüm geliştirme ve test süreci tamamlandıktan sonra doldurulacaktır.

## Takım Rol Dağılımı

| Rol | Sorumluluk |
|---|---|
| Proje lideri / entegrasyon | `main.c`, `Makefile`, genel akış ve README düzeni |
| Thread pool sorumlusu | `thread_pool.c`, `thread_pool.h`, worker yaşam döngüsü ve shutdown |
| Kuyruk ve job modeli sorumlusu | `job_queue.c`, `job_queue.h`, `job.c`, `job.h` |
| İş tipleri, loglama ve metrik sorumlusu | `tasks.c`, `logger.c`, `metrics.c` ve temel test girdileri |

Bu dağılım başlangıç önerisidir. Geliştirme sırasında ihtiyaçlara göre görev paylaşımı güncellenebilir.

## Karşılaşılan Problemler

Bu bölüm geliştirme ve test süreci tamamlandıktan sonra doldurulacaktır.
