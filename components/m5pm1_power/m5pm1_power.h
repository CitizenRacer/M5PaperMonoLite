#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace m5pm1_power {

class M5PM1Power : public PollingComponent, public i2c::I2CDevice {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;

  // The PMIC needs to be configured before the M5IOE1 and display stack.
  float get_setup_priority() const override { return setup_priority::BUS - 1.0f; }

  void set_battery_voltage_sensor(sensor::Sensor *sensor) { this->battery_voltage_ = sensor; }

  // Standard ESPHome float output range, 0.0 = off and 1.0 = full brightness.
  void set_frontlight_brightness(float state);

  bool is_usb_present();
  void power_off();

 protected:
  bool read_u8_(uint8_t reg, uint8_t &value);
  bool write_u8_(uint8_t reg, uint8_t value);
  bool update_bits_(uint8_t reg, uint8_t clear_mask, uint8_t set_mask);
  bool configure_frontlight_();

  sensor::Sensor *battery_voltage_{nullptr};
};

}  // namespace m5pm1_power
}  // namespace esphome
