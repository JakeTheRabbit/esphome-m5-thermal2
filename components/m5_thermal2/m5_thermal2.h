#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace m5_thermal2 {

// M5Stack Unit Thermal2 (SKU U149) — MLX90640 110deg FoV behind an on-unit
// ESP32-PICO-D4 co-processor. The host does NOT speak the raw MLX90640 protocol;
// it reads a processed summary register map from the unit's MCU.
//
// Register map (from m5stack/M5Unit-Thermal2, src/M5_Thermal2.h):
//   0x00  status        : [0]=button bitmask [1]=alarm [4]=device_id_0 [5]=device_id_1
//                         [6]=fw_major [7]=fw_minor
//   0x08  config (16B)  : [0]=i2c_addr [1]=~i2c_addr [2]=function_ctrl
//                         [3]=refresh_rate(0..7) [4]=noise_filter(0..15)
//                         [8]=monitor_area(w|h<<4) [9]=alarm_enable
//                         [10..11]=buzzer_freq [12]=buzzer_vol [13..15]=led r,g,b
//   0x20  low alarm (8B), 0x30 high alarm (8B):
//                         [0..1]=threshold_raw [2..3]=buzzer_freq [4]=interval*10ms
//                         [5..7]=led r,g,b
//   0x6E  refresh_ctrl : [0] bit0 = new-frame-ready, [1]=subpage
//   0x70  overview(16B): median,average,most_diff (u16 each), most_diff x,y (u8),
//                         lowest(u16) lowest x,y(u8), highest(u16) highest x,y(u8)
//                         followed by 768 bytes of raw pixel data (not read here).
// Temperature: celsius = raw / 128 - 64.

class M5Thermal2 : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // --- codegen configuration setters ---
  void set_refresh_rate(uint8_t code) { this->refresh_rate_ = code & 0x07; }
  void set_noise_filter(uint8_t level) { this->noise_filter_ = level & 0x0F; }
  void set_monitor_area(uint8_t w, uint8_t h) { this->monitor_area_ = (w & 0x0F) | ((h & 0x0F) << 4); }

  void set_average_sensor(sensor::Sensor *s) { this->average_sensor_ = s; }
  void set_median_sensor(sensor::Sensor *s) { this->median_sensor_ = s; }
  void set_max_sensor(sensor::Sensor *s) { this->max_sensor_ = s; }
  void set_min_sensor(sensor::Sensor *s) { this->min_sensor_ = s; }
  void set_diff_sensor(sensor::Sensor *s) { this->diff_sensor_ = s; }
  void set_hotspot_x_sensor(sensor::Sensor *s) { this->hotspot_x_sensor_ = s; }
  void set_hotspot_y_sensor(sensor::Sensor *s) { this->hotspot_y_sensor_ = s; }
  void set_button_sensor(binary_sensor::BinarySensor *s) { this->button_sensor_ = s; }

  // --- runtime controls (call from lambdas / automations) ---
  void set_led(uint8_t r, uint8_t g, uint8_t b);
  void led_on();
  void led_off();
  void set_buzzer(uint16_t freq, uint8_t volume);
  void buzzer_on();
  void buzzer_off();
  void set_refresh_rate_live(uint8_t code);
  void set_noise_filter_live(uint8_t level);
  // interval is in units of 10 ms (5..255); buzzer_freq 0 = silent.
  void set_alarm_high(float temp_c, uint8_t interval, uint16_t buzzer_freq, uint8_t r, uint8_t g, uint8_t b);
  void set_alarm_low(float temp_c, uint8_t interval, uint16_t buzzer_freq, uint8_t r, uint8_t g, uint8_t b);
  void alarm_on(uint8_t mask = 0xFF);
  void alarm_off(uint8_t mask = 0xFF);

 protected:
  static float raw_to_c(uint16_t raw) { return (float) raw / 128.0f - 64.0f; }
  static uint16_t c_to_raw(float c);
  bool write_config_();

  uint8_t refresh_rate_{5};     // 16 Hz
  uint8_t noise_filter_{8};
  uint8_t monitor_area_{0xBF};  // width 15 | (height 11 << 4)
  uint8_t config_[16]{};        // cached config block 0x08..0x17
  uint8_t fw_major_{0};
  uint8_t fw_minor_{0};

  sensor::Sensor *average_sensor_{nullptr};
  sensor::Sensor *median_sensor_{nullptr};
  sensor::Sensor *max_sensor_{nullptr};
  sensor::Sensor *min_sensor_{nullptr};
  sensor::Sensor *diff_sensor_{nullptr};
  sensor::Sensor *hotspot_x_sensor_{nullptr};
  sensor::Sensor *hotspot_y_sensor_{nullptr};
  binary_sensor::BinarySensor *button_sensor_{nullptr};
};

}  // namespace m5_thermal2
}  // namespace esphome
