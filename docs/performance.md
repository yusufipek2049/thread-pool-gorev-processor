# Performans ve Test Raporu

Geliştirme aşamasında oluşturduğumuz farklı test senaryolarının (Unit, Entegrasyon, Stres ve Gelişmiş Analizler) sonuçları

## 1. Unit Test Sonuçları (`make test_runner`)

| Test Edilen Modül | Fonksiyon / Özellik | Başarı Durumu (Pass/Fail) |
| :---: | :---: | :--- |
| `job_queue` | Kuyruğa ekleme (push) & alma (pop) | Pass |
| `job_queue` | Tam kuyruk (full queue) koruması | Pass |
| `job_queue` | Thread-safe yapısı | Pass |
| `tasks` | Asal sayı kontrolü (is_prime) | Pass |
| `tasks` | Satır sayımı (count_lines) | Pass |
| `tasks` | Dosya Hash alma (hash_file) | Pass |

## 2. Entegrasyon ve Stres Testleri (`make stress_test`)

Uygulamanın farklı iş yükleri (jobs dosyaları) ve worker thread sayıları altındaki performans tablosu:

### 2.1 Karışık Ölçekli Test (`jobs_mixed.txt`)

| Worker Sayısı | Toplam İş | Başarılı | Başarısız | Toplam Süre | İş/Saniye | Gözlem |
| :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| 2 | 7 | 7 | 0 | 0.0084 | 836.45 | Toplam iş sayısı çok az (7 adet) olduğu için en optimum sonucu düşük thread sayısı verdi. |
| 4 | 7 | 7 | 0 | 0.0140 | 500.06 | Thread başlatma ve yok etme maliyeti işin kendi süresini aştığı için performans düştü. | 
| 8 | 7 | 7 | 0 | 0.0118 |  594.64 | Görev sayısından daha fazla thread açılması performans kaybına yol açtı. |

### 2.2 Yoğun Stres Testi (`jobs_stress.txt` - 10.000 İşlem)

| Worker Sayısı | Toplam İş | Başarılı | Başarısız | Toplam Süre | İş/Saniye |
| :---: | :---: | :---: | :---: | :---: | :--- |
| 1 | 10000 | 10000 | 0 | 8.1588 | 1225.68 |
| 2 | 10000 | 10000 | 0 | 7.9023 | 1265.45 |
| 4 | 10000 | 10000 | 0 | 8.7080 | 1148.36 |
| 8 | 10000 | 10000 | 0 | 11.1546 | 896.49 |
| 16 | 10000 | 10000 | 0 | 16.6241 | 601.54 |

Görevlerin işlenme süresi çok kısa olduğu için, ekrana log yazdırırken oluşan Mutex kilitlenmeleri (Lock Contention) ve Context Switch maliyeti, thread sayısı arttıkça programı yavaşlatmıştır. Daha büyük Gb'lerce işlemlerde multithearding gerçek performasını gösterir.  

## 3. Gelişmiş Analiz Sonuçları

| Kullanılan Araç | Test Edilen Senaryo | Sonuç  |
| :--- | :--- | :--- |
| **Valgrind** | Bellek Sızıntısı (Memory Leak) | Pass |
| **Thread Sanitizer** | Eşzamanlılık Kilitlenmesi (Race Condition) | Pass |
| **Gcov** | Kod Test Kapsamı (Code Coverage) Yüzdesi | 70.15% |
