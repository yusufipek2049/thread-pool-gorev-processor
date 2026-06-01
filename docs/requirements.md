# Gereksinim Özeti

Bu belge, projenin ana beklentilerini kısa ve güncel şekilde toplar. Ayrıntılı proje tanımı kökteki gereksinim dokümanından takip edilebilir; buradaki liste ise mevcut branch'in doğrulanan davranışına göre düzenlenmiştir.

## Temel Beklentiler

- Program C dili ve POSIX/Linux API ile geliştirilecektir.
- Worker thread sayısı ve kuyruk kapasitesi komut satırından verilebilecektir.
- İşler ortak bir FIFO kuyruğuna eklenecektir.
- Kuyruk işlemleri mutex ile korunacaktır.
- Worker thread'ler kuyruk boşken busy waiting yapmayacak, condition variable ile bekleyecektir.
- `FILE_HASH`, `LINE_COUNT` ve `PRIME_CHECK` iş tipleri desteklenecektir.
- Girdi dosyasında boş satırlar ve yorum satırları atlanacaktır.
- Geçersiz iş satırları programı durdurmayacak, hata olarak loglanıp başarısız iş sayısına yansıtılacaktır.
- Program kapanışta kuyrukta bekleyen işleri tamamlayacak, thread'leri join edecek ve kaynakları temizleyecektir.

## Test ve Analiz Beklentileri

- `make test` unit testleri ve kısa entegrasyon testini çalıştırmalıdır.
- `make valgrind` bellek sızıntısı ve temel bellek hatalarını kontrol etmelidir.
- `make coverage` satır kapsamını raporlamalıdır.
- `make tsan` ThreadSanitizer ile data race kontrolü için hazır tutulmalıdır.

## Güncel Doğrulama Durumu

- Unit ve entegrasyon testleri geçmiştir.
- Valgrind sonucu temizdir: leak yok, bellek hatası yok.
- Coverage son ölçümde `%85.48` seviyesine çıkarılmıştır.
- Eski raporda ThreadSanitizer sonucu `Pass` olarak kayıtlıdır.
- Güncel WSL2 tekrarında ThreadSanitizer `unexpected memory mapping` hatasıyla başlamadan durmaktadır. Bu nedenle yeni TSan doğrulaması bu ortamda tamamlanamamıştır.
