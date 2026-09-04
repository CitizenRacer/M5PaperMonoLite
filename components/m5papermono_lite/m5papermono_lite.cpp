#include "m5papermono_lite.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5papermono_lite {

static const char *const TAG = "m5papermono_lite";

void M5PaperMonoLite::setup() {
  if (this->touch_power_pin_ == nullptr || this->touch_reset_pin_ == nullptr) {
    ESP_LOGE(TAG, "Touch power/reset pins are not configured");
    this->mark_failed();
    return;
  }

  // M5PaperMono: M5IOE1 PIN13 is touch power; PIN6 is touch reset.
  // The M5IOE1 ESPHome component uses zero-based pin numbers, so the package
  // maps those to 12 and 5 respectively.
  this->touch_power_pin_->setup();
  this->touch_reset_pin_->setup();

  this->touch_power_pin_->digital_write(true);
  delay(5);

  this->touch_reset_pin_->digital_write(false);
  delay(10);
  this->touch_reset_pin_->digital_write(true);
  delay(20);

  ESP_LOGCONFIG(TAG, "FT6336G touch power enabled and reset released");
}

void M5PaperMonoLite::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PaperMono Lite board glue:");
  if (this->is_failed())
    ESP_LOGE(TAG, "  Touch power/reset initialization failed");
}

}  // namespace m5papermono_lite
}  // namespace esphome
