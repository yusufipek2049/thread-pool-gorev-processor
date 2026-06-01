# Performans ve Test Notları

Bu doküman, `testing-loging-branch` üzerinde yapılan son test turunu özetler. Sonuçlar WSL2 ortamında alınmıştır; bu yüzden özellikle sanitizer ve performans ölçümlerinde ortam etkisi dikkate alınmalıdır. Aynı komutlar farklı bir Linux ortamında küçük farklarla sonuç verebilir.

## Test Kapsamı

Coverage'ı artırmak için test runner genişletildi. Yeni testler yalnızca mutlu yolu değil, hatalı parametreleri ve kapanış senaryolarını da çalıştırıyor:

- `job_queue_create(0)` ve negatif kapasite kontrolleri
- `NULL` queue ile `push`, `pop`, `is_empty`, `is_full`, `size`, `capacity`
- `job_queue_size()` ve `job_queue_capacity()` doğrulamaları
- `thread_pool_create()` için geçersiz worker/kuyruk değerleri
- `thread_pool_submit(NULL, job)` davranışı
- `shutdown` sonrası submit denemesi
- Thread pool içinde başarısız job çalıştırma yolu
- CLI tarafında `--help`, hatalı argümanlar, eksik değerler, bulunamayan input dosyası ve karışık geçerli/geçersiz job dosyası

`make test` sonucu:

| Test | Sonuç |
|---|---|
| `test_job_queue_basic` | Pass |
| `test_job_queue_edges` | Pass |
| `test_prime_check` | Pass |
| `test_line_count` | Pass |
| `test_file_hash` | Pass |
| `test_invalid_job_type` | Pass |
| `test_thread_pool_edges` | Pass |

Test runner sonucu `7 / 7 passed`. Kısa entegrasyon testinde `tests/jobs_mixed.txt` ile 7 işin tamamı başarıyla işlendi.

## Coverage Sonucu

Coverage hedefi artık sadece birim testleri değil, CLI hata yollarını da çalıştırıyor. Önceki ölçümde toplam satır kapsamı `%69.49` idi. Yeni ölçümde kapsam `%85.48` seviyesine çıktı.

| Dosya | Satır Kapsamı |
|---|---:|
| `src/job.c` | 100.00% |
| `src/job_queue.c` | 80.95% |
| `src/logger.c` | 100.00% |
| `src/main.c` | 83.68% |
| `src/metrics.c` | 100.00% |
| `src/tasks.c` | 100.00% |
| `src/thread_pool.c` | 72.88% |
| **Toplam** | **85.48%** |

En belirgin artış `main.c`, `job_queue.c` ve `thread_pool.c` tarafında oldu. Hâlâ düşük kalan bölüm `thread_pool.c`; burada thread oluşturma hatası gibi sistem seviyesinde zor tetiklenen hata yolları doğal olarak kapsanmıyor.

## Valgrind

Valgrind hem `test_runner` hem de kısa entegrasyon testi üzerinde temiz geçti.

| Hedef | Sonuç |
|---|---|
| `./test_runner` | 0 leak, 0 error |
| `./threadpool_app -t 4 -q 32 -i tests/jobs_mixed.txt` | 0 leak, 0 error |

Özet olarak tüm heap blokları serbest bırakıldı ve Valgrind bellek hatası raporlamadı.

## Thread Sanitizer

Önceki test turunda ThreadSanitizer hedefi çalışmış ve rapora `Pass` olarak işlenmişti. O turdaki gelişmiş analiz özeti şöyleydi:

| Araç | Senaryo | Eski Sonuç |
|---|---|---|
| Thread Sanitizer | Eşzamanlılık / race condition kontrolü | Pass |

Bu eski sonuç, o dönemde TSan altında belirgin bir race condition raporlanmadığını gösterir.

ThreadSanitizer hedefi tekrar denendi, ancak bu WSL2 ortamında tamamlanamadı:

```text
FATAL: ThreadSanitizer: unexpected memory mapping
```

Bu hata testler başlamadan sanitizer runtime içinde oluşuyor. Bu yüzden güncel WSL2 turu eski `Pass` sonucunu geçersiz kılmaz; sadece bu ortamda yeni bir TSan doğrulaması yapılamadığını gösterir. Doğru yorum şu olmalıdır: geçmiş TSan turu `Pass`, güncel WSL2 tekrarında ise analiz runtime uyumsuzluğu nedeniyle tamamlanamadı.

## Stres Testi

Stres testi `tests/jobs_stress.txt` içindeki 10.000 görevle çalıştırıldı. Her ölçümde kuyruk kapasitesi 128 olarak bırakıldı.

```bash
./threadpool_app -t <worker_sayisi> -q 128 -i tests/jobs_stress.txt
```

| Worker | Toplam İş | Başarılı | Başarısız | Duvar Süresi | Raporlanan İş Süresi | Ortalama İş Süresi | Duvar Süresine Göre İş/Saniye |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 1 | 10000 | 10000 | 0 | 4.84 s | 4.7051 s | 0.0005 s | 2066.1 |
| 2 | 10000 | 10000 | 0 | 2.25 s | 4.3644 s | 0.0004 s | 4444.4 |
| 4 | 10000 | 10000 | 0 | 1.30 s | 4.9759 s | 0.0005 s | 7692.3 |
| 8 | 10000 | 10000 | 0 | 1.42 s | 11.0651 s | 0.0011 s | 7042.3 |

Bu ölçümde en iyi duvar süresi 4 worker ile alındı. 8 worker hâlâ hızlı, ancak bu iş yükünde 4 worker'ın gerisine düştü. Bunun temel nedeni işlerin çok kısa sürmesi; thread sayısı arttıkça loglama, mutex bekleme ve context switch maliyeti daha görünür hale geliyor.

Programın raporladığı "toplam iş süresi" worker sürelerinin toplamıdır. Bu değer duvar süresiyle aynı şey değildir. Paralel çalışan işler üst üste bindiği için duvar süresi düşerken raporlanan toplam iş süresi artabilir.

## Kısa Yorum

Son test turunda fonksiyonel testler geçti, Valgrind temiz kaldı ve coverage anlamlı şekilde arttı. Stres testinde 10.000 iş kayıpsız tamamlandı. ThreadSanitizer için geçmiş raporda `Pass` sonucu var; ancak mevcut WSL2 ortamında aynı analiz tekrar çalıştırılamadığı için güncel doğrulama notu ayrıca tutuldu.
