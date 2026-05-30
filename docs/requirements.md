# Gereksinim Ozeti

Bu belge proje baslangicinda ana gereksinimleri kisa sekilde hatirlatmak icin tutulur. Ayrintili gereksinimler proje kokundeki gereksinim dokumanindan takip edilebilir.

## Temel Beklentiler

- Program C dili ve POSIX/Linux API ile gelistirilecektir.
- Worker thread sayisi kullanici tarafindan belirlenebilir olacaktir.
- Isler ortak bir FIFO kuyruguna eklenecektir.
- Kuyruk islemleri mutex ile korunacaktir.
- Worker thread'ler kuyruk bosken busy waiting yapmayacaktir.
- En az `FILE_HASH`, `LINE_COUNT` ve `PRIME_CHECK` is tipleri desteklenecektir.
- Program kapanista bekleyen isleri tamamlayip kaynaklari temizleyecektir.
