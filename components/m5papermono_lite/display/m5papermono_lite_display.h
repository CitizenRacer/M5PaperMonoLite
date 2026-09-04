#pragma once

#include "esphome/components/epaper_spi/epaper_spi_mono.h"

namespace esphome {
namespace m5papermono_lite {

class M5PaperMonoLiteDisplay : public epaper_spi::EPaperMono {
 public:
  M5PaperMonoLiteDisplay()
      : EPaperMono("M5PaperMono Lite SSD1677", 800, 480, INIT_SEQUENCE_, sizeof(INIT_SEQUENCE_)) {}

 protected:
  // M5Stack's current PaperMono monochrome init sequence. RAM data-entry mode
  // is set in set_window() because this panel reverses its gate/Y direction.
  inline static constexpr uint8_t INIT_SEQUENCE_[] = {
      0x18, 0x01, 0x80,
      0x0C, 0x05, 0xAE, 0xC7, 0xC3, 0xC0, 0x80,
      0x01, 0x03, 0xDF, 0x01, 0x02,
      0x3C, 0x01, 0x01,
  };

  void set_window() override;
  bool transfer_data() override;
  void refresh_screen(bool partial) override;
  void power_on() override {}
  void power_off() override;
  void deep_sleep() override;

  uint8_t transfer_plane_{0};
  bool transfer_started_{false};
};

}  // namespace m5papermono_lite
}  // namespace esphome
