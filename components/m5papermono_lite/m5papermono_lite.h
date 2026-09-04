#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace m5papermono_lite {

class M5PaperMonoLite : public Component {
 public:
  void setup() override;
  void dump_config() override;

  // M5IOE1 itself sets up at setup_priority::IO. Run immediately after it but
  // before the FT5x06 touchscreen and display components initialize.
  float get_setup_priority() const override { return setup_priority::IO - 1.0f; }

  void set_touch_power_pin(GPIOPin *pin) { this->touch_power_pin_ = pin; }
  void set_touch_reset_pin(GPIOPin *pin) { this->touch_reset_pin_ = pin; }

 protected:
  GPIOPin *touch_power_pin_{nullptr};
  GPIOPin *touch_reset_pin_{nullptr};
};

}  // namespace m5papermono_lite
}  // namespace esphome
