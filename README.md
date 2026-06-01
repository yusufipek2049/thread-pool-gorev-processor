# Paralel Görev İşleyici

Bu proje, C dili ve POSIX/Linux API kullanılarak geliştirilen thread pool tabanlı bir görev işleyici örneğidir. Amaç, ortak bir iş kuyruğuna eklenen işleri birden fazla worker thread ile çalıştırmak ve bunu yaparken senkronizasyon, hata yönetimi ve basit performans ölçümü gibi sistem programlama konularını uygulamalı olarak göstermektir.

Bu branch, başlangıç iskeletinin üstüne gerçek worker thread döngüsü, condition variable ile bekleme/uyandırma, dosyadan görev okuma ve temel metrik raporu ekler. Kod hâlâ ders projesi kapsamındadır; ilerleyen aşamada test senaryoları ve performans raporu genişletilecektir.

## Amaç

Program, girdi dosyasından okunan görevleri FIFO mantığıyla çalışan ortak bir kuyruğa ekler. Worker thread'ler bu kuyruktan görev alır, görevin tipine göre ilgili fonksiyonu çalıştırır ve sonucu loglar.

Bu çalışmada özellikle şu konular hedeflenir:

- `pthread` ile worker thread oluşturma
- Mutex ile paylaşılan veriyi koruma
- Condition variable ile busy waiting yapmadan bekleme
- Producer-consumer modelini uygulama
- Dosya okuma, hata kontrolü ve kaynak temizliği
- Program sonunda kısa bir performans özeti üretme

## Tasarım

Uygulama dört ana parçadan oluşur:

- `main.c`: Komut satırı argümanlarını okur, görev dosyasını parse eder, thread pool'u başlatır ve kapanışı yönetir.
- `thread_pool.c`: Worker thread'leri oluşturur, görev kabul eder, kuyruğu worker'lara dağıtır ve shutdown sürecini yürütür.
- `job_queue.c`: FIFO kuyruk işlemlerini sağlar.
- `tasks.c`: Desteklenen görev tiplerini çalıştırır.

Worker thread'ler kuyruk boşken condition variable üzerinde bekler. Yeni görev eklendiğinde bir worker uyandırılır. Kapanış sırasında tüm worker'lara sinyal gönderilir, kuyrukta kalan işler tamamlanır ve thread'ler `pthread_join()` ile beklenir.

Kuyruk dolu olduğunda görev düşürülmez. `thread_pool_submit()` kuyrukta yer açılmasını bekler. Böylece kullanıcı kaç görev verdiyse raporda da o görevlerin karşılığı izlenebilir.

## Kullanılan Sistem Programlama Kavramları

- `pthread_create()` ve `pthread_join()`
- `pthread_mutex_t`
- `pthread_cond_t`
- Producer-consumer modeli
- FIFO circular buffer
- File I/O
- `clock_gettime()` ile süre ölçümü
- Thread-safe loglama ve metrik güncelleme
- Graceful shutdown

## Kurulum

Linux veya POSIX uyumlu bir ortamda `gcc`, `make` ve pthread desteği yeterlidir.

```bash
make
```

Derleme sonunda proje kökünde `threadpool_app` dosyası oluşur.

## Çalıştırma Adımları

Varsayılan örnek görevlerle çalıştırmak için:

```bash
./threadpool_app
```

Girdi dosyası ile çalıştırmak için:

```bash
./threadpool_app --threads 4 --queue-size 32 --input tests/jobs_mixed.txt
```

Kısa kullanım da desteklenir:

```bash
./threadpool_app -t 4 -q 32 tests/jobs_mixed.txt
```

Yardım çıktısı:

```bash
./threadpool_app --help
```

## Girdi Dosyası Formatı

Her satır bir görevi temsil eder:

```text
<GOREV_TIPI> <PARAMETRE>
```

Örnek:

```text
PRIME 999983
LINE_COUNT tests/sample1.txt
FILE_HASH tests/sample2.txt
PRIME 1234567
```

Boş satırlar ve `#` ile başlayan yorum satırları atlanır. Geçersiz satırlar programı durdurmaz; hata olarak loglanır ve metriklerde başarısız görev olarak sayılır.

## Desteklenen İş Tipleri

| İş Tipi | Açıklama |
|---|---|
| `PRIME` | Verilen pozitif tam sayının asal olup olmadığını kontrol eder. |
| `LINE_COUNT` | Verilen dosyadaki satır sayısını hesaplar. |
| `FILE_HASH` | Verilen dosya için basit bir hash değeri üretir. |

`PRIME` görevlerinde sayı doğrulaması yapılır. Örneğin `PRIME abc` geçerli kabul edilmez; `abc` değeri `0` gibi yorumlanıp başarılı görev sayılmaz.

## Testler

Projeye eklenmiş olan unit test ve entegrasyon testlerini çalıştırmak için `make test` komutunu kullanabilirsiniz. Bu komut hem `test_runner` uygulamasını derler ve çalıştırır, hem de ana uygulamayı farklı senaryolarla test eder:

```bash
make test
```

Sadece unit testleri (job_queue, tasks vb.) izole olarak derleyip çalıştırmak isterseniz:

```bash
make test_runner
./test_runner
```

### Entegrasyon ve Stres Testleri

Eğer uygulamanın `jobs_mixed.txt` ve `jobs_stress.txt` dosyalarıyla gerçek bir yük altında nasıl çalıştığını görmek isterseniz, hazır olarak eklenmiş stres testi komutunu kullanabilirsiniz:

```bash
make stress_test
```
Bu komut, uygulamayı farklı worker thread sayıları ve yoğun iş dosyalarındaki tüm görevleri işler. Uygulamanın performansını baştan sona analiz etmek için harika bir araçtır.

```bash
make mixed_jobs_test
```
Bu komut uygulamayı farklı worker thread sayıları ve karışık iş dosyalarındaki tüm görevleri işler. Uygulamanın performansını baştan sona analiz etmek için harika bir araçtır.

Elle denenebilecek örnek senaryolar:

```bash
./threadpool_app --threads 4 --queue-size 32 --input tests/jobs_mixed.txt
./threadpool_app -t 4 -q 1 tests/jobs_mixed.txt
```

## Performans Değerlendirmesi

Program sonunda şu bilgiler raporlanır:

- Worker sayısı
- Toplam iş sayısı
- Başarılı iş sayısı
- Başarısız iş sayısı
- Başarı oranı
- Toplam iş süresi
- Ortalama iş süresi
- Saniye başına iş sayısı

Daha ayrıntılı performans tablosu `docs/performance.md` içinde farklı worker sayılarıyla doldurulacaktır.

## Takım Rol Dağılımı

| Rol | Sorumluluk |
|---|---|
| Proje lideri / entegrasyon | `main.c`, `Makefile`, genel akış |
| Thread pool sorumlusu | `thread_pool.c`, worker döngüsü, shutdown |
| Kuyruk ve job modeli sorumlusu | `job_queue.c`, `job.c`, ilgili header dosyaları |
| Görevler, loglama ve metrikler | `tasks.c`, `logger.c`, `metrics.c`, test girdileri |

## Son Düzenlemeler

Bu branch'te özellikle şu hatalar ele alındı:

- Metrik sayaçları mutex ile korunur hale getirildi.
- Log yazımı mutex ile seri hale getirildi.
- Shutdown bayrağı kilit altında kontrol edildi.
- Kuyruk doluyken görev düşürmek yerine boş yer bekleme mantığı eklendi.
- `PRIME` parametreleri `strtol()` ile doğrulanır hale getirildi.
- Geçersiz görev satırları başarısız iş olarak metriklere yansıtıldı.

## Karşılaşılan Problemler

Geliştirme sırasında en önemli riskler race condition, log satırlarının birbirine karışması ve hatalı girdilerin başarılı görev gibi görünmesiydi. Bu branch'te bu noktalar temel seviyede düzeltildi. Kalan işler daha çok test kapsamının genişletilmesi ve performans metriklerinin ayrıntılandırılması tarafındadır.
