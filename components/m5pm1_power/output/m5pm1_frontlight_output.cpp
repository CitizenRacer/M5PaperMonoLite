#include "m5pm1_frontlight_output.h"

#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1_power {

static const char *const TAG = "m5pm1_power.output";

void M5PM1FrontlightOutput::setup() {
  this->parent_->set_frontlight_brightness(0.0f);
}

void M5PM1FrontlightOutput::write_state(float state) {
  this->parent_->set_frontlight_brightness(state);
}

void M5PM1FrontlightOutput::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PaperMono frontlight output");
}

}  // namespace m5pm1_power
}  // namespace esphome
