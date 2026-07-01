#include "m5_thermal2.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace m5_thermal2 {

static const char *const TAG = "m5_thermal2";

// register indices
static const uint8_t REG_STATUS = 0x00;
static const uint8_t REG_CONFIG = 0x08;
static const uint8_t REG_LOW_ALARM = 0x20;
static const uint8_t REG_HIGH_ALARM = 0x30;
static const uint8_t REG_OVERVIEW = 0x70;

// status block identity bytes
static const uint8_t DEVICE_ID_0 = 0x90;
static const uint8_t DEVICE_ID_1 = 0x64;

// function_ctrl bits (config_[2])
static const uint8_t FUNC_BUZZER = 0x01;
static const uint8_t FUNC_LED = 0x02;
static const uint8_t FUNC_CONTINUOUS = 0x04;

uint16_t M5Thermal2::c_to_raw(float c) {
  int v = (int) ((c + 64.0f) * 128.0f);
  if (v < 0)
    v = 0;
  if (v > 65535)
    v = 65535;
  return (uint16_t) v;
}

bool M5Thermal2::write_config_() { return this->write_register(REG_CONFIG, this->config_, 16) == i2c::ERROR_OK; }

void M5Thermal2::setup() {
  // 1) Identify the unit by reading its status block (8 bytes from 0x00).
  //    The unit's MCU needs >100 ms to boot, so retry for ~400 ms before failing
  //    (mirrors the 16x16ms retry in the M5 library's begin()).
  uint8_t status[8];
  bool found = false;
  for (uint8_t i = 0; i < 20; i++) {
    if (this->read_register(REG_STATUS, status, 8) == i2c::ERROR_OK && status[4] == DEVICE_ID_0 &&
        status[5] == DEVICE_ID_1) {
      found = true;
      break;
    }
    delay(20);
  }
  if (!found) {
    ESP_LOGE(TAG, "Unit Thermal2 not found at 0x%02X (no/!ID response). Check 5V wiring and i2c scan.",
             this->address_);
    this->mark_failed();
    return;
  }
  this->fw_major_ = status[6];
  this->fw_minor_ = status[7];

  // 2) Read current config so we preserve the unit's stored I2C address etc.,
  //    then apply our settings and force continuous-refresh mode.
  if (this->read_register(REG_CONFIG, this->config_, 16) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Failed to read config block");
    this->mark_failed();
    return;
  }
  this->config_[2] |= FUNC_CONTINUOUS;
  this->config_[3] = this->refresh_rate_;
  this->config_[4] = this->noise_filter_;
  this->config_[8] = this->monitor_area_;
  if (!this->write_config_()) {
    ESP_LOGE(TAG, "Failed to write config block");
    this->mark_failed();
    return;
  }
}

void M5Thermal2::update() {
  if (this->is_failed())
    return;

  // Button / alarm status. Latched event bits (everything but bit0) are cleared
  // by writing the value back to the status register, mirroring the M5 library.
  uint8_t st[2];
  if (this->read_register(REG_STATUS, st, 2) == i2c::ERROR_OK) {
    uint8_t button = st[0];
    if (this->button_sensor_ != nullptr)
      this->button_sensor_->publish_state((button & 0x01) != 0);
    if (button & ~0x01u)
      this->write_register(REG_STATUS, &button, 1);
  }

  // Overview block: processed min/max/avg/median + hotspot coordinates.
  uint8_t ov[16];
  if (this->read_register(REG_OVERVIEW, ov, 16) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Overview read failed");
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();

  uint16_t median = ov[0] | (ov[1] << 8);
  uint16_t average = ov[2] | (ov[3] << 8);
  uint16_t most_diff = ov[4] | (ov[5] << 8);
  uint16_t lowest = ov[8] | (ov[9] << 8);
  uint16_t highest = ov[12] | (ov[13] << 8);
  uint8_t high_x = ov[14];
  uint8_t high_y = ov[15];

  if (this->average_sensor_ != nullptr)
    this->average_sensor_->publish_state(raw_to_c(average));
  if (this->median_sensor_ != nullptr)
    this->median_sensor_->publish_state(raw_to_c(median));
  if (this->max_sensor_ != nullptr)
    this->max_sensor_->publish_state(raw_to_c(highest));
  if (this->min_sensor_ != nullptr)
    this->min_sensor_->publish_state(raw_to_c(lowest));
  if (this->diff_sensor_ != nullptr)
    this->diff_sensor_->publish_state((float) most_diff / 128.0f);  // a delta — no -64 offset
  if (this->hotspot_x_sensor_ != nullptr)
    this->hotspot_x_sensor_->publish_state(high_x);
  if (this->hotspot_y_sensor_ != nullptr)
    this->hotspot_y_sensor_->publish_state(high_y);
}

void M5Thermal2::dump_config() {
  ESP_LOGCONFIG(TAG, "M5Stack Unit Thermal2:");
  LOG_I2C_DEVICE(this);
  ESP_LOGCONFIG(TAG, "  Firmware: v%u.%u", this->fw_major_, this->fw_minor_);
  ESP_LOGCONFIG(TAG, "  Refresh rate code: %u (0=0.5Hz .. 7=64Hz)", this->refresh_rate_);
  ESP_LOGCONFIG(TAG, "  Noise filter: %u", this->noise_filter_);
  LOG_UPDATE_INTERVAL(this);
  if (this->is_failed())
    ESP_LOGE(TAG, "  Communication with Unit Thermal2 failed");
}

// --- runtime controls ---

void M5Thermal2::set_led(uint8_t r, uint8_t g, uint8_t b) {
  this->config_[13] = r;
  this->config_[14] = g;
  this->config_[15] = b;
  this->config_[2] |= FUNC_LED;
  this->write_config_();
}
void M5Thermal2::led_on() {
  this->config_[2] |= FUNC_LED;
  this->write_config_();
}
void M5Thermal2::led_off() {
  this->config_[2] &= ~FUNC_LED;
  this->write_config_();
}
void M5Thermal2::set_buzzer(uint16_t freq, uint8_t volume) {
  this->config_[10] = freq & 0xFF;
  this->config_[11] = freq >> 8;
  this->config_[12] = volume;
  this->write_config_();
}
void M5Thermal2::buzzer_on() {
  this->config_[2] |= FUNC_BUZZER;
  this->write_config_();
}
void M5Thermal2::buzzer_off() {
  this->config_[2] &= ~FUNC_BUZZER;
  this->write_config_();
}
void M5Thermal2::set_refresh_rate_live(uint8_t code) {
  this->refresh_rate_ = code & 0x07;
  this->config_[3] = this->refresh_rate_;
  this->write_config_();
}
void M5Thermal2::set_noise_filter_live(uint8_t level) {
  this->noise_filter_ = level & 0x0F;
  this->config_[4] = this->noise_filter_;
  this->write_config_();
}
void M5Thermal2::set_alarm_high(float temp_c, uint8_t interval, uint16_t buzzer_freq, uint8_t r, uint8_t g, uint8_t b) {
  uint16_t thr = c_to_raw(temp_c);
  uint8_t a[8] = {(uint8_t) (thr & 0xFF), (uint8_t) (thr >> 8), (uint8_t) (buzzer_freq & 0xFF),
                  (uint8_t) (buzzer_freq >> 8), interval, r, g, b};
  this->write_register(REG_HIGH_ALARM, a, 8);
}
void M5Thermal2::set_alarm_low(float temp_c, uint8_t interval, uint16_t buzzer_freq, uint8_t r, uint8_t g, uint8_t b) {
  uint16_t thr = c_to_raw(temp_c);
  uint8_t a[8] = {(uint8_t) (thr & 0xFF), (uint8_t) (thr >> 8), (uint8_t) (buzzer_freq & 0xFF),
                  (uint8_t) (buzzer_freq >> 8), interval, r, g, b};
  this->write_register(REG_LOW_ALARM, a, 8);
}
void M5Thermal2::alarm_on(uint8_t mask) {
  this->config_[9] |= mask;
  this->write_config_();
}
void M5Thermal2::alarm_off(uint8_t mask) {
  this->config_[9] &= ~mask;
  this->write_config_();
}

}  // namespace m5_thermal2
}  // namespace esphome
