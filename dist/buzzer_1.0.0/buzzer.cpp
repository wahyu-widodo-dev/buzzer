#include "include/buzzer.h"

#include "esp_log.h"
#include "esp_timer.h"

namespace {
constexpr const char *TAG = "buzzer";

constexpr Buzzer::ToneStep kClick[] = {
    {3200, 10},
};
constexpr Buzzer::ToneStep kButtonPress[] = {
    {2400, 25},
};
constexpr Buzzer::ToneStep kAlarm[] = {
    {1800, 150},
    {0, 150},
};
constexpr Buzzer::ToneStep kDoubleClick[] = {
    {3200, 10},
    {0, 70},
    {3200, 10},
};
constexpr Buzzer::ToneStep kSuccess[] = {
    {1600, 70},
    {2200, 100},
};
constexpr Buzzer::ToneStep kError[] = {
    {500, 150},
    {0, 50},
    {500, 150},
};
constexpr Buzzer::ToneStep kWarning[] = {
    {1800, 100},
    {0, 100},
    {1800, 100},
};
constexpr Buzzer::ToneStep kNotification[] = {
    {2000, 60},
    {0, 50},
    {2600, 80},
};
constexpr Buzzer::ToneStep kStartup[] = {
    {800, 50},
    {1200, 50},
    {1600, 80},
};
} // namespace

Buzzer::~Buzzer()
{
    if (initialized_) {
        const esp_err_t err = deinit();
        if (err != ESP_OK) {
            log_error("deinit pada destructor", err);
        }
    }
}

bool Buzzer::init()
{
    return init(Config{}) == ESP_OK;
}

esp_err_t Buzzer::init(const Config &config)
{
    if (initialized_) {
        ESP_LOGE(TAG, "Buzzer sudah diinisialisasi");
        return ESP_ERR_INVALID_STATE;
    }
    if (config.initial_frequency_hz == 0 ||
        config.initial_volume_percent > 100 ||
        static_cast<unsigned>(config.duty_resolution) < 1 ||
        static_cast<unsigned>(config.duty_resolution) > 20) {
        ESP_LOGE(TAG, "Konfigurasi buzzer tidak valid");
        return ESP_ERR_INVALID_ARG;
    }

    config_ = config;
    volume_ = config.initial_volume_percent;

    ledc_timer_config_t timer_config = {};
    timer_config.speed_mode = config_.speed_mode;
    timer_config.duty_resolution = config_.duty_resolution;
    timer_config.timer_num = config_.timer;
    timer_config.freq_hz = config_.initial_frequency_hz;
    timer_config.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        log_error("konfigurasi LEDC timer", err);
        return err;
    }

    ledc_channel_config_t channel_config = {};
    channel_config.gpio_num = config_.gpio_num;
    channel_config.speed_mode = config_.speed_mode;
    channel_config.channel = config_.channel;
    channel_config.intr_type = LEDC_INTR_DISABLE;
    channel_config.timer_sel = config_.timer;
    channel_config.duty = 0;
    channel_config.hpoint = 0;
    err = ledc_channel_config(&channel_config);
    if (err != ESP_OK) {
        log_error("konfigurasi LEDC channel", err);
        return err;
    }

    initialized_ = true;
    current_frequency_hz_ = config_.initial_frequency_hz;
    ESP_LOGI(TAG, "Siap pada GPIO %d, LEDC timer %d channel %d",
             static_cast<int>(config_.gpio_num),
             static_cast<int>(config_.timer),
             static_cast<int>(config_.channel));
    return ESP_OK;
}

esp_err_t Buzzer::deinit()
{
    if (!initialized_) {
        return ESP_OK;
    }

    const esp_err_t stop_err = stop();
    if (stop_err != ESP_OK) {
        return stop_err;
    }

    const esp_err_t err = ledc_stop(config_.speed_mode, config_.channel, 0);
    if (err != ESP_OK) {
        log_error("menghentikan LEDC channel", err);
        return err;
    }

    initialized_ = false;
    current_frequency_hz_ = 0;
    return ESP_OK;
}

void Buzzer::set_volume(int volume)
{
    volume_ = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
    if (initialized_ && step_count_ != 0) {
        const esp_err_t err = apply_step();
        if (err != ESP_OK) {
            log_error("mengubah volume", err);
            const esp_err_t mute_err = set_output(current_frequency_hz_, false);
            if (mute_err != ESP_OK) {
                log_error("membisukan output setelah error volume", mute_err);
            }
            current_pattern_ = TonePattern::None;
            step_count_ = 0;
            repeat_sequence_ = false;
        }
    }
}

int Buzzer::get_volume() const
{
    return volume_;
}

esp_err_t Buzzer::play(TonePattern pattern)
{
    switch (pattern) {
    case TonePattern::None:
        return stop();
    case TonePattern::Click:
        return start_sequence(kClick, sizeof(kClick) / sizeof(kClick[0]),
                              false, pattern);
    case TonePattern::ButtonPress:
        return start_sequence(kButtonPress,
                              sizeof(kButtonPress) / sizeof(kButtonPress[0]),
                              false, pattern);
    case TonePattern::AlarmPattern:
        return start_sequence(kAlarm, sizeof(kAlarm) / sizeof(kAlarm[0]),
                              true, pattern);
    case TonePattern::DoubleClick:
        return start_sequence(kDoubleClick,
                              sizeof(kDoubleClick) / sizeof(kDoubleClick[0]),
                              false, pattern);
    case TonePattern::Success:
        return start_sequence(kSuccess, sizeof(kSuccess) / sizeof(kSuccess[0]),
                              false, pattern);
    case TonePattern::Error:
        return start_sequence(kError, sizeof(kError) / sizeof(kError[0]),
                              false, pattern);
    case TonePattern::Warning:
        return start_sequence(kWarning, sizeof(kWarning) / sizeof(kWarning[0]),
                              false, pattern);
    case TonePattern::Notification:
        return start_sequence(kNotification,
                              sizeof(kNotification) / sizeof(kNotification[0]),
                              false, pattern);
    case TonePattern::Startup:
        return start_sequence(kStartup, sizeof(kStartup) / sizeof(kStartup[0]),
                              false, pattern);
    case TonePattern::Custom:
    default:
        ESP_LOGE(TAG, "Pola tone tidak dikenal atau tidak dapat dipanggil langsung");
        return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t Buzzer::play_tone(uint32_t frequency_hz, uint32_t duration_ms)
{
    const ToneStep step = {frequency_hz, duration_ms};
    return play_sequence(&step, 1, false);
}

esp_err_t Buzzer::play_sequence(const ToneStep *steps,
                                size_t step_count,
                                bool repeat)
{
    return start_sequence(steps, step_count, repeat, TonePattern::Custom);
}

void Buzzer::tick()
{
    if (!initialized_ || current_pattern_ == TonePattern::None ||
        step_count_ == 0) {
        return;
    }

    const int64_t now_us = esp_timer_get_time();
    const uint64_t duration_us =
        static_cast<uint64_t>(steps_[step_index_].duration_ms) * 1000U;
    if (now_us - step_started_us_ < static_cast<int64_t>(duration_us)) {
        return;
    }

    if (step_index_ + 1 < step_count_) {
        ++step_index_;
    } else if (repeat_sequence_) {
        step_index_ = 0;
    } else {
        const esp_err_t err = stop();
        if (err != ESP_OK) {
            log_error("mengakhiri pola tone", err);
        }
        return;
    }

    step_started_us_ = now_us;
    const esp_err_t err = apply_step();
    if (err != ESP_OK) {
        log_error("memainkan langkah tone", err);
        const esp_err_t mute_err = set_output(current_frequency_hz_, false);
        if (mute_err != ESP_OK) {
            log_error("membisukan output setelah error", mute_err);
        }
        current_pattern_ = TonePattern::None;
        step_count_ = 0;
        repeat_sequence_ = false;
    }
}

void Buzzer::click()
{
    if (volume_ == 0 || current_pattern_ == TonePattern::AlarmPattern) {
        return;
    }
    const esp_err_t err = play(TonePattern::Click);
    if (err != ESP_OK) {
        log_error("memainkan click", err);
    }
}

void Buzzer::button()
{
    if (volume_ == 0 || current_pattern_ == TonePattern::AlarmPattern) {
        return;
    }
    const esp_err_t err = play(TonePattern::ButtonPress);
    if (err != ESP_OK) {
        log_error("memainkan tombol", err);
    }
}

void Buzzer::alarm()
{
    if (volume_ == 0) {
        return;
    }
    const esp_err_t err = play(TonePattern::AlarmPattern);
    if (err != ESP_OK) {
        log_error("memainkan alarm", err);
    }
}

void Buzzer::stop_alarm()
{
    const esp_err_t err = stop();
    if (err != ESP_OK) {
        log_error("menghentikan alarm", err);
    }
}

esp_err_t Buzzer::stop()
{
    current_pattern_ = TonePattern::None;
    step_count_ = 0;
    step_index_ = 0;
    repeat_sequence_ = false;
    if (!initialized_) {
        return ESP_OK;
    }
    return set_output(current_frequency_hz_, false);
}

Buzzer::TonePattern Buzzer::current_pattern() const
{
    return current_pattern_;
}

bool Buzzer::is_initialized() const
{
    return initialized_;
}

uint32_t Buzzer::calculate_duty() const
{
    const uint32_t full_scale = 1U << static_cast<unsigned>(config_.duty_resolution);
    return (full_scale * static_cast<uint32_t>(volume_)) / 200U;
}

esp_err_t Buzzer::apply_step()
{
    const ToneStep &step = steps_[step_index_];
    if (step.frequency_hz == 0) {
        return set_output(current_frequency_hz_, false);
    }

    current_frequency_hz_ = step.frequency_hz;
    return set_output(step.frequency_hz, volume_ > 0);
}

esp_err_t Buzzer::set_output(uint32_t frequency_hz, bool enabled)
{
    if (!initialized_) {
        return ESP_ERR_INVALID_STATE;
    }

    if (enabled) {
        if (frequency_hz == 0) {
            ESP_LOGE(TAG, "Frekuensi tone harus lebih besar dari nol");
            return ESP_ERR_INVALID_ARG;
        }
        const esp_err_t frequency_err =
            ledc_set_freq(config_.speed_mode, config_.timer, frequency_hz);
        if (frequency_err != ESP_OK) {
            ESP_LOGE(TAG, "LEDC gagal mengatur frekuensi %u Hz: %s",
                     static_cast<unsigned>(frequency_hz),
                     esp_err_to_name(frequency_err));
            return frequency_err;
        }
    }

    const uint32_t duty = enabled ? calculate_duty() : 0;
    esp_err_t err = ledc_set_duty(config_.speed_mode, config_.channel, duty);
    if (err != ESP_OK) {
        log_error("mengatur duty LEDC", err);
        return err;
    }
    err = ledc_update_duty(config_.speed_mode, config_.channel);
    if (err != ESP_OK) {
        log_error("memperbarui duty LEDC", err);
    }
    return err;
}

esp_err_t Buzzer::start_sequence(const ToneStep *steps,
                                 size_t step_count,
                                 bool repeat,
                                 TonePattern pattern)
{
    if (!initialized_) {
        ESP_LOGE(TAG, "Inisialisasi buzzer harus berhasil sebelum memainkan tone");
        return ESP_ERR_INVALID_STATE;
    }
    if (steps == nullptr || step_count == 0 || step_count > MaxToneSteps) {
        ESP_LOGE(TAG, "Jumlah langkah tone harus berada pada rentang 1..%u",
                 static_cast<unsigned>(MaxToneSteps));
        return ESP_ERR_INVALID_ARG;
    }
    if (current_pattern_ == TonePattern::AlarmPattern &&
        pattern != TonePattern::AlarmPattern) {
        return ESP_ERR_INVALID_STATE;
    }
    for (size_t i = 0; i < step_count; ++i) {
        if (steps[i].duration_ms == 0) {
            ESP_LOGE(TAG, "Durasi langkah tone tidak boleh nol");
            return ESP_ERR_INVALID_ARG;
        }
    }

    for (size_t i = 0; i < step_count; ++i) {
        steps_[i] = steps[i];
    }
    step_count_ = step_count;
    step_index_ = 0;
    repeat_sequence_ = repeat;
    current_pattern_ = pattern;
    step_started_us_ = esp_timer_get_time();

    const esp_err_t err = apply_step();
    if (err != ESP_OK) {
        const esp_err_t mute_err = set_output(current_frequency_hz_, false);
        if (mute_err != ESP_OK) {
            log_error("membisukan output setelah error", mute_err);
        }
        current_pattern_ = TonePattern::None;
        step_count_ = 0;
        repeat_sequence_ = false;
        return err;
    }
    return ESP_OK;
}

void Buzzer::log_error(const char *operation, esp_err_t error)
{
    ESP_LOGE(TAG, "Gagal %s: %s", operation, esp_err_to_name(error));
}