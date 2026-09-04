#pragma once

#include "esphome/components/output/float_output.h"
#include "../m5pm1_power.h"

namespace esphome {
namespace m5pm1_power {

class M5PM1FrontlightOutput : public output::FloatOutput,
                              public Component,
                              public Parented<M5PM1Power> {
 public:
  void setup() override;
  void dump_config() override;

 protected:
  void write_state(float state) override;
};

}  // namespace m5pm1_power
}  // namespace esphome
