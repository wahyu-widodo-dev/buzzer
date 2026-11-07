# Buzzer

Komponen C++ untuk mengendalikan **passive buzzer** menggunakan PWM LEDC pada
ESP-IDF. Komponen menyediakan pola bunyi siap pakai dan pemutaran urutan tone
yang dapat dikustomisasi. Pemutaran bersifat non-blocking dan dijalankan dari
task aplikasi melalui pemanggilan `tick()` berkala.

## Fitur

- Tone untuk klik, tombol, alarm, status sukses/error, peringatan, notifikasi,
  dan startup.
- Urutan tone kustom hingga 16 langkah, termasuk langkah senyap.
- Frekuensi, durasi, volume, GPIO, dan resource LEDC dapat dikonfigurasi.
- Operasi pemutaran tidak menunggu atau memblokir selama durasi tone.
- Validasi argumen dan pelaporan error ESP-IDF melalui `esp_err_t`.
- Tidak bergantung pada timer atau header privat aplikasi.

## Persyaratan perangkat

Komponen ini untuk **passive buzzer** yang menghasilkan bunyi saat menerima
sinyal dengan frekuensi tertentu. Active buzzer yang sudah memiliki osilator
internal umumnya hanya perlu diberi level DC dan tidak memerlukan pola tone
dari driver ini.

Hubungkan buzzer ke GPIO output yang sesuai untuk board dan target ESP Anda.
Pastikan rangkaian buzzer tidak mengambil arus melebihi kemampuan GPIO; gunakan
transistor/driver eksternal jika diperlukan. Perhatikan polaritas buzzer dan
tegangan kerja pada datasheet komponen.

## Instalasi

Komponen ini ditujukan untuk ESP Component Registry. Setelah komponen
dipublikasikan dengan namespace `wahyu-widodo-dev` dan nama `buzzer`, tambahkan
dependency ke manifest proyek ESP-IDF, yaitu `idf_component.yml`:

```yaml
dependencies:
  wahyu-widodo-dev/buzzer:
    version: "^1.0.0"
```

Rentang `^1.0.0` menerima rilis kompatibel mulai versi 1.0.0, tetapi tidak
memasukkan versi major 2.x. Sesuaikan rentangnya jika proyek memerlukan versi
tertentu. Simpan manifest tersebut di direktori proyek ESP-IDF, biasanya
sejajar dengan `CMakeLists.txt` utama.

Cara lain untuk menambahkan dependency adalah menjalankan perintah berikut
dari direktori proyek:

```sh
idf.py add-dependency "wahyu-widodo-dev/buzzer^1.0.0"
```

Kemudian build proyek:

```sh
idf.py build
```

ESP Component Manager akan mencari dan mengunduh komponen beserta dependency
yang diperlukan, lalu mencatat versinya pada `dependencies.lock`. Pastikan
proyek memiliki koneksi internet saat dependency pertama kali diselesaikan.

> **Catatan:** Registry hanya dapat mengambil komponen setelah rilisnya berhasil
> dipublikasikan. Sebelum `wahyu-widodo-dev/buzzer` tersedia di Registry,
> pengunduhan melalui manifest belum dapat dilakukan. Untuk pengembangan lokal
> sebelum publikasi, komponen dapat sementara diletakkan di direktori
> `components/buzzer` proyek.

Komponen memerlukan ESP-IDF **5.3.0 atau lebih baru**.

Komponen terdaftar sebagai C++ dan menyediakan header publik `buzzer.h`.
Tambahkan `buzzer` ke dependency komponen aplikasi di `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES buzzer
)
```

## Mulai cepat

```cpp
#include "buzzer.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static Buzzer buzzer;

extern "C" void app_main(void)
{
    Buzzer::Config config;
    config.gpio_num = GPIO_NUM_4; // Ganti dengan GPIO output board Anda.
    config.initial_volume_percent = 50;
    ESP_ERROR_CHECK(buzzer.init(config));

    ESP_ERROR_CHECK(buzzer.play(Buzzer::TonePattern::Success));

    while (true) {
        buzzer.tick();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
```

Panggil `tick()` berulang dari task selama pola bunyi perlu berjalan. Contoh
menggunakan jeda task 1 ms; aplikasi dapat memakai periode lain yang sesuai
dengan kebutuhan dan beban sistem. Pemanggilan `tick()` yang lebih jarang
membuat pergantian langkah tone terlambat, tetapi tidak menyebabkan task tidur
selama bunyi.

## Konfigurasi hardware

Semua field `Buzzer::Config`:

| Field | Default | Keterangan |
|---|---:|---|
| `gpio_num` | `GPIO_NUM_39` | GPIO output buzzer. Periksa dukungan pin pada chip/board. |
| `speed_mode` | `LEDC_LOW_SPEED_MODE` | Mode LEDC yang digunakan. |
| `channel` | `LEDC_CHANNEL_0` | Channel LEDC untuk output buzzer. |
| `timer` | `LEDC_TIMER_0` | Timer LEDC untuk menghasilkan frekuensi. |
| `duty_resolution` | `LEDC_TIMER_10_BIT` | Resolusi duty LEDC; nilai yang tersedia bergantung target. |
| `initial_frequency_hz` | `2700` | Frekuensi awal timer saat inisialisasi; harus lebih besar dari nol. |
| `initial_volume_percent` | `50` | Volume awal, 0..100. |

**Pilih GPIO yang sesuai dengan board Anda**; jangan menganggap GPIO default
tersedia atau aman untuk setiap target.

LEDC timer dibagi bersama oleh semua channel yang memilih timer dan mode sama.
Driver mengubah frekuensi timer saat memainkan setiap langkah bernada.
Karena itu, berikan timer khusus kepada buzzer jika channel lain pada timer yang
sama memerlukan frekuensi tetap. Jangan biarkan komponen/periferal lain
mengonfigurasi ulang channel LEDC atau timer yang digunakan buzzer.

## Pola tone bawaan

Jalankan pola melalui `play(Buzzer::TonePattern::...)`:

| Pola | Urutan frekuensi dan waktu | Perulangan |
|---|---|---|
| `Click` | 3200 Hz selama 10 ms | Tidak |
| `ButtonPress` | 2400 Hz selama 25 ms | Tidak |
| `AlarmPattern` | 1800 Hz selama 150 ms, lalu senyap 150 ms | Ya, sampai dihentikan |
| `DoubleClick` | 3200 Hz 10 ms, senyap 70 ms, 3200 Hz 10 ms | Tidak |
| `Success` | 1600 Hz 70 ms, lalu 2200 Hz 100 ms | Tidak |
| `Error` | 500 Hz 150 ms, senyap 50 ms, 500 Hz 150 ms | Tidak |
| `Warning` | 1800 Hz 100 ms, senyap 100 ms, 1800 Hz 100 ms | Tidak |
| `Notification` | 2000 Hz 60 ms, senyap 50 ms, 2600 Hz 80 ms | Tidak |
| `Startup` | 800 Hz 50 ms, 1200 Hz 50 ms, 1600 Hz 80 ms | Tidak |
| `None` | Menghentikan pola aktif dan membisukan output | — |
| `Custom` | Penanda untuk urutan yang diberikan melalui API kustom; tidak dapat dipakai dengan `play()` | — |

Frekuensi dan durasi adalah nilai target. Buzzer, rangkaian penggerak, resolusi
timer, konfigurasi clock, dan jadwal pemanggilan `tick()` dapat memengaruhi
hasil akustik aktual.

### Contoh memainkan pola

```cpp
ESP_ERROR_CHECK(buzzer.play(Buzzer::TonePattern::Notification));
```

Pola yang sedang berjalan akan digantikan oleh pola baru. Jika alarm aktif,
pola non-alarm baru ditolak dengan `ESP_ERR_INVALID_STATE`; hentikan alarm
terlebih dahulu jika ingin memainkan bunyi lain.

## Urutan tone kustom

Satu langkah direpresentasikan oleh `Buzzer::ToneStep`:

- `frequency_hz`: frekuensi PWM dalam Hz; `0` berarti senyap.
- `duration_ms`: durasi langkah dalam milidetik; tidak boleh nol.

Satu urutan menerima 1..`Buzzer::MaxToneSteps` langkah (maksimum 16).
`play_sequence()` menyalin langkah ke dalam objek buzzer sebelum kembali,
sehingga array sumber tidak perlu tetap hidup setelah pemanggilan.

```cpp
const Buzzer::ToneStep melody[] = {
    {1000, 100},
    {0, 40},
    {1500, 180},
};

ESP_ERROR_CHECK(buzzer.play_sequence(melody, 3));
```

Parameter `repeat` opsional bernilai `false` secara default. Gunakan `true`
untuk mengulang urutan sampai dihentikan:

```cpp
ESP_ERROR_CHECK(buzzer.play_sequence(melody, 3, true));
// Di kemudian hari:
ESP_ERROR_CHECK(buzzer.stop());
```

Untuk satu nada atau satu jeda:

```cpp
ESP_ERROR_CHECK(buzzer.play_tone(2200, 120));
ESP_ERROR_CHECK(buzzer.play_tone(0, 50)); // Senyap 50 ms.
```

Pastikan frekuensi dapat dihasilkan oleh kombinasi target, clock, dan resolusi
LEDC. Jika LEDC menolak frekuensi pada langkah ketika dimainkan, driver
mencatat error, menghentikan pola, dan mencoba membisukan output.

## Volume

```cpp
buzzer.set_volume(70);
int volume = buzzer.get_volume();
```

Volume adalah persentase duty maksimum gelombang PWM, bukan pengukuran volume
akustik dalam dB. Nilai di bawah 0 dibatasi menjadi 0 dan nilai di atas 100
dibatasi menjadi 100. Pada volume 100, duty mendekati 50% dari rentang timer;
ini menghasilkan gelombang kotak yang sesuai untuk passive buzzer.

Volume `0` membisukan output. Timer pola tetap berjalan, sehingga pola dapat
selesai ketika disenyapkan. Mengubah volume saat tone aktif langsung
memperbarui duty langkah saat itu.

## API dan perilaku

### Inisialisasi

```cpp
Buzzer buzzer;
bool ready = buzzer.init();
```

`init()` menggunakan seluruh nilai default `Buzzer::Config` dan
mengembalikan `true` jika berhasil. Jika aplikasi perlu memilih pin atau
resource LEDC:

```cpp
Buzzer::Config config;
config.gpio_num = GPIO_NUM_4;
config.channel = LEDC_CHANNEL_1;
config.timer = LEDC_TIMER_1;
config.initial_volume_percent = 65;
esp_err_t err = buzzer.init(config);
```

`init(config)` mengembalikan `ESP_OK` jika berhasil atau error ESP-IDF jika
konfigurasi LEDC gagal. Inisialisasi kedua pada objek yang sama, tanpa
`deinit()` di antaranya, mengembalikan `ESP_ERR_INVALID_STATE`.

### Pemutaran dan status

```cpp
esp_err_t err = buzzer.play(Buzzer::TonePattern::Success);
buzzer.tick(); // Panggil berkala dari task aplikasi.
Buzzer::TonePattern active = buzzer.current_pattern();
bool ready = buzzer.is_initialized();
```

`play()` mengembalikan `ESP_ERR_INVALID_STATE` jika buzzer belum berhasil
diinisialisasi, `ESP_ERR_INVALID_ARG` untuk nilai pola yang tidak didukung
atau `Custom`, serta error driver LEDC jika output gagal diatur. Pola
non-berulang otomatis berhenti setelah langkah terakhir; `current_pattern()`
kemudian bernilai `None`.

`tick()` bernilai `void` dan tidak memblokir selama durasi tone. Panggil dari
task, bukan ISR. Satu pemanggilan memproses paling banyak satu pergantian
langkah; jika task lama tidak berjalan, pola dapat tertunda.

### API singkat

Metode berikut tersedia:

| Metode | Perilaku |
|---|---|
| `init()` | Inisialisasi konfigurasi default; mengembalikan `bool`. |
| `init(const Config&)` | Inisialisasi dengan konfigurasi; mengembalikan `esp_err_t`. |
| `deinit()` | Hentikan PWM dan setel GPIO ke level idle; aman dipanggil saat belum aktif. |
| `set_volume(int)` | Atur volume 0..100; nilai dibatasi ke rentang tersebut. |
| `get_volume() const` | Ambil volume saat ini. |
| `play(TonePattern)` | Jalankan pola bawaan. |
| `play_tone(uint32_t, uint32_t)` | Jalankan satu frekuensi dan durasi. |
| `play_sequence(const ToneStep*, size_t, bool)` | Jalankan urutan kustom, opsional berulang. |
| `tick()` | Perbarui state pola; panggil rutin dari task. |
| `click()` | Mainkan pola `Click`; tidak mengembalikan status error. |
| `button()` | Mainkan pola `ButtonPress`; tidak mengembalikan status error. |
| `alarm()` | Mulai pola alarm berulang; tidak mengembalikan status error. |
| `stop_alarm()` | Hentikan alarm atau pola yang sedang berjalan. |
| `stop()` | Hentikan pola apa pun; mengembalikan `esp_err_t`. |
| `current_pattern() const` | Ambil pola yang aktif atau `None`. |
| `is_initialized() const` | Periksa status inisialisasi objek. |

Shortcut `click()`, `button()`, dan `alarm()` mencatat error ke log karena
signature-nya tidak mengembalikan status. Gunakan `play()` jika aplikasi perlu
memeriksa hasil pemanggilan.

### Prioritas pola

- Pola baru menggantikan pola non-alarm yang sedang berlangsung.
- Selama `AlarmPattern` aktif, `play()` untuk pola lain dan urutan kustom
  mengembalikan `ESP_ERR_INVALID_STATE`.
- `stop()` menghentikan alarm maupun pola lainnya; `stop_alarm()` juga
  menghentikan pola yang sedang berjalan.
- `TonePattern::None` pada `play()` sama dengan menghentikan pola.

### Penghentian dan kepemilikan resource

```cpp
ESP_ERROR_CHECK(buzzer.stop());
ESP_ERROR_CHECK(buzzer.deinit());
```

`deinit()` membisukan output dan menghentikan channel LEDC. Objek tidak
menghapus atau mengembalikan konfigurasi timer LEDC, karena timer merupakan
resource bersama pada periferal LEDC. Jangan mengubah timer/channel yang sama
melalui driver lain selama buzzer digunakan. Destructor mencoba menjalankan
`deinit()` jika objek masih terinisialisasi.

## Thread-safety

Satu instance `Buzzer` **tidak thread-safe**. Panggil semua metodenya dari satu
task atau lindungi akses dengan mekanisme sinkronisasi aplikasi. Jangan
memanggil metode dari ISR. Driver tidak membuat task, software timer, atau
callback internal.

## Batasan dan pemecahan masalah

- **Tidak ada bunyi:** pastikan buzzer passive, GPIO benar, buzzer mendapat
  driver listrik yang sesuai, `init()` berhasil, `tick()` dipanggil, dan
  volume lebih dari nol.
- **Bunyi tidak sesuai frekuensi:** periksa dukungan frekuensi/resolusi LEDC
  target dan pastikan timer tidak dipakai atau dikonfigurasi ulang periferal
  lain.
- **Pola terlambat:** tingkatkan frekuensi pemanggilan `tick()` atau kurangi
  pekerjaan yang memblokir task pemanggil.
- **GPIO tidak tersedia:** pilih GPIO output yang sesuai dengan pinout board
  dan hindari pin yang dipakai periferal/flash/PSRAM pada target.
- **Error konfigurasi:** periksa nilai enum LEDC yang didukung target, serta
  pastikan channel dan timer belum dimiliki konfigurasi lain yang bertentangan.

## Lisensi

Komponen ini dirilis di bawah lisensi MIT. Lihat [LICENSE](./LICENSE).
