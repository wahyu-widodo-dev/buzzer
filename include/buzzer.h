#pragma once

#include <cstddef>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"

/**
 * @brief Driver non-blocking untuk passive buzzer berbasis LEDC.
 *
 * Panggil tick() secara berkala dari task aplikasi untuk menggerakkan pola
 * tone. Tone Click, ButtonPress, dan AlarmPattern mempertahankan frekuensi dan
 * durasi dari implementasi awal.
 */
class Buzzer
{
public:
    enum class TonePattern : uint8_t {
        None = 0,
        Click,
        ButtonPress,
        AlarmPattern,
        DoubleClick,
        Success,
        Error,
        Warning,
        Notification,
        Startup,
        Custom,
    };

    struct Config {
        gpio_num_t gpio_num = GPIO_NUM_39;
        ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;
        ledc_channel_t channel = LEDC_CHANNEL_0;
        ledc_timer_t timer = LEDC_TIMER_0;
        ledc_timer_bit_t duty_resolution = LEDC_TIMER_10_BIT;
        uint32_t initial_frequency_hz = 2700;
        uint8_t initial_volume_percent = 50;
    };

    struct ToneStep {
        uint32_t frequency_hz; /**< 0 berarti jeda/senyap. */
        uint32_t duration_ms;
    };

    enum : size_t {
        MaxToneSteps = 16,
    };

    Buzzer() = default;
    ~Buzzer();

    Buzzer(const Buzzer &) = delete;
    Buzzer &operator=(const Buzzer &) = delete;

    /** Inisialisasi dengan GPIO39 dan konfigurasi LEDC default. */
    bool init();

    /** Inisialisasi dengan konfigurasi perangkat pilihan aplikasi. */
    esp_err_t init(const Config &config);

    /** Hentikan output PWM dan setel GPIO ke level idle. */
    esp_err_t deinit();

    /** Atur volume 0..100 persen; nilai di luar rentang dibatasi. */
    void set_volume(int volume);
    int get_volume() const;

    /** Jalankan satu pola bawaan. Panggil tick() berkala untuk progres. */
    esp_err_t play(TonePattern pattern);

    /** Putar satu tone atau urutan tone kustom yang disalin oleh driver. */
    esp_err_t play_tone(uint32_t frequency_hz, uint32_t duration_ms);
    esp_err_t play_sequence(const ToneStep *steps,
                            size_t step_count,
                            bool repeat = false);

    /** Lanjutkan pola aktif; panggil rutin dari task aplikasi. */
    void tick();

    /** API kompatibilitas untuk pola lama. */
    void click();
    void button();
    void alarm();
    void stop_alarm();

    /** Hentikan pola apa pun yang sedang berjalan. */
    esp_err_t stop();

    TonePattern current_pattern() const;
    bool is_initialized() const;

private:
    Config config_{};
    ToneStep steps_[MaxToneSteps]{};
    size_t step_count_ = 0;
    size_t step_index_ = 0;
    int volume_ = 50;
    int64_t step_started_us_ = 0;
    uint32_t current_frequency_hz_ = 0;
    TonePattern current_pattern_ = TonePattern::None;
    bool repeat_sequence_ = false;
    bool initialized_ = false;

    uint32_t calculate_duty() const;
    esp_err_t apply_step();
    esp_err_t set_output(uint32_t frequency_hz, bool enabled);
    esp_err_t start_sequence(const ToneStep *steps,
                             size_t step_count,
                             bool repeat,
                             TonePattern pattern);
    static void log_error(const char *operation, esp_err_t error);
};
