# Tasarim Notlari

Bu dokuman proje baslangicinda mimariyi ortak bir zemine oturtmak icin hazirlandi. Uygulama gelistikce detaylar burada guncellenebilir.

## Genel Akis

Program once komut satiri ayarlarini okuyacak, ardindan thread pool ve ortak is kuyrugunu baslatacaktir. Girdi dosyasindaki her satir bir is olarak yorumlanacak ve kuyruga eklenecektir. Worker thread'ler kuyruktan is alarak ilgili is fonksiyonunu calistiracaktir.

## Moduller

- `main.c`: Program girisi, arguman okuma ve genel akisi yonetir.
- `thread_pool.c`: Worker thread yasam dongusu, is kabul etme ve kapanis surecinden sorumludur.
- `job_queue.c`: FIFO kuyruk islemlerini saglar.
- `job.c`: Is tipi yardimci fonksiyonlarini tutar.
- `tasks.c`: Desteklenen is tiplerini calistirir.
- `metrics.c`: Basit performans verilerini toplar.
- `logger.c`: Bilgi ve hata mesajlarini standart formata yakin sekilde yazar.

## Thread Pool Mantigi

Baslangic iskeletinde thread pool arayuzu sabitlenmistir. Ilk surumde `submit` cagrisi isi senkron olarak calistirir. Sonraki adimda ayni arayuz korunarak worker thread'ler, condition variable ve shutdown bayragi eklenecektir.

## Kuyruk Mantigi

Kuyruk FIFO mantigiyla calisir. `push` yeni isi kuyrugun sonuna ekler, `pop` ise en eski isi alir. Kuyruk yapisi mutex ile korunur; bu sayede ileride birden fazla producer veya worker kullanildiginda veri bozulmasi engellenir.

## Kapanis Mekanizmasi

Kapanis istegi geldiginde thread pool yeni is kabul etmeyi durduracaktir. Gercek paralel surumde bekleyen worker thread'ler uyandirilacak, kuyruktaki isler tamamlanacak ve tum thread'ler `pthread_join()` ile beklenecektir.
