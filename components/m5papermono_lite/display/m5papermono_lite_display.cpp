#include "m5papermono_lite_display.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5papermono_lite {

static const char *const TAG = "m5papermono_lite.display";

void M5PaperMonoLiteDisplay::set_window() {
  // Full-refresh-only bring-up driver. Sending both complete RAM planes gives
  // the controller a deterministic baseline and avoids generic SSD1677 gate
  // addressing, which is wrong for the PaperMono panel.
  this->x_low_ = 0;
  this->x_high_ = this->width_;
  this->y_low_ = 0;
  this->y_high_ = this->height_;

  // M5GFX PaperMono mapping: X increments, gate/Y decrements.
  this->cmd_data(0x11, {0x01});

  // RAM X: 0..799.
  this->cmd_data(0x44, {0x00, 0x00, 0x1F, 0x03});
  this->cmd_data(0x4E, {0x00, 0x00});

  // RAM Y/gates: 479..0.
  this->cmd_data(0x45, {0xDF, 0x01, 0x00, 0x00});
  this->cmd_data(0x4F, {0xDF, 0x01});
}

bool M5PaperMonoLiteDisplay::transfer_data() {
  // M5Stack's PaperMono full B/W update writes the same image into both SSD1677
  // RAM planes. ESPHome's generic EPaperMono clears the second plane instead,
  // which is not the PaperMono sequence.
  if (!this->transfer_started_) {
    this->set_window();
    this->command(this->transfer_plane_ == 0 ? 0x24 : 0x26);
    this->current_data_index_ = 0;
    this->transfer_started_ = true;
  }

  const uint32_t start_time = millis();
  uint8_t row[100];  // 800 monochrome pixels / 8.

  this->start_data_();
  while (this->current_data_index_ < this->height_) {
    const size_t offset = this->current_data_index_ * this->row_width_;
    for (size_t i = 0; i < this->row_width_; i++) {
      row[i] = this->buffer_[offset + i];
    }
    this->write_array(row, this->row_width_);
    this->current_data_index_++;

    if (millis() - start_time > epaper_spi::MAX_TRANSFER_TIME) {
      this->disable();
      return false;
    }
  }
  this->disable();

  this->current_data_index_ = 0;
  this->transfer_started_ = false;

  if (this->transfer_plane_ == 0) {
    this->transfer_plane_ = 1;
    return false;
  }

  this->transfer_plane_ = 0;
  return true;
}

void M5PaperMonoLiteDisplay::refresh_screen(bool partial) {
  // Partial refresh is deliberately not exposed yet. The first implementation
  // follows M5Stack's current M5GFX full monochrome path:
  //   DISP_UPDATE_CTRL1 = bypass red RAM comparison
  //   DISP_UPDATE_CTRL2 = power-on + FULL built-in waveform
  //   MASTER_ACTIVATION
  // The state machine waits for BUSY before proceeding to power_off().
  if (partial) {
    ESP_LOGW(TAG, "Partial refresh requested but not supported; using full refresh");
  }
  this->cmd_data(0x21, {0x40});
  this->cmd_data(0x22, {0xF4});
  this->command(0x20);
}

void M5PaperMonoLiteDisplay::power_off() {
  // Match M5GFX: shut down the SSD1677 analog section and clock after a full
  // refresh. The ESPHome state machine waits for BUSY before deep_sleep().
  this->cmd_data(0x22, {0x03});
  this->command(0x20);
}

void M5PaperMonoLiteDisplay::deep_sleep() {
  // SSD1677 Deep Sleep Mode 1. A hardware reset (already performed by
  // EPaperMono on the next update) is required to leave this mode.
  this->cmd_data(0x10, {0x01});
}

}  // namespace m5papermono_lite
}  // namespace esphome
