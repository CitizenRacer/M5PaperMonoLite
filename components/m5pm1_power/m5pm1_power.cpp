// M5PM1 support for M5PaperMono / M5PaperMono Lite.
//
// Derived in part from PFalko/m5stack-papercolor-esphome (MIT, Copyright
// (c) 2026 PFalko). PaperMono-specific power and frontlight behavior follows
// M5Stack's MIT-licensed M5GFX implementation.

#include "m5pm1_power.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace m5pm1_power {

static const char *const TAG = "m5pm1_power";

static constexpr uint16_t M5PM1_DEVICE_ID = 0x2050;

// M5PM1 registers used here.
static constexpr uint8_t REG_DEVICE_ID = 0x00;
static constexpr uint8_t REG_PWR_CFG = 0x06;
static constexpr uint8_t REG_I2C_CFG = 0x09;
static constexpr uint8_t REG_WDT_CNT = 0x0A;
static constexpr uint8_t REG_SYS_CMD = 0x0C;
static constexpr uint8_t REG_GPIO_DRV = 0x13;
static constexpr uint8_t REG_GPIO_FUNC = 0x16;
static constexpr uint8_t REG_VBAT_L = 0x22;
static constexpr uint8_t REG_VIN_L = 0x24;
static constexpr uint8_t REG_PWM0_L = 0x30;
static constexpr uint8_t REG_PWM0_H = 0x31;
static constexpr uint8_t REG_PWM_FREQ_L = 0x34;

// M5GFX initializes PaperMono with CHG, DCDC, LDO and LED control enabled.
static constexpr uint8_t PAPERMONO_PWR_CFG_BITS = 0x17;

bool M5PM1Power::read_u8_(uint8_t reg, uint8_t &value) {
  return this->read_register(reg, &value, 1) == i2c::ERROR_OK;
}

bool M5PM1Power::write_u8_(uint8_t reg, uint8_t value) {
  return this->write_register(reg, &value, 1) == i2c::ERROR_OK;
}

bool M5PM1Power::update_bits_(uint8_t reg, uint8_t clear_mask, uint8_t set_mask) {
  uint8_t value = 0;
  if (!this->read_u8_(reg, value))
    return false;
  value = static_cast<uint8_t>((value & ~clear_mask) | set_mask);
  return this->write_u8_(reg, value);
}

bool M5PM1Power::configure_frontlight_() {
  // PaperMono frontlight is M5PM1 GPIO3 in PWM mode.
  // 0x13 bit3=0 => push-pull.
  if (!this->update_bits_(REG_GPIO_DRV, 1 << 3, 0))
    return false;

  // 0x16 bits7:6=11 => PWM function for GPIO3.
  if (!this->update_bits_(REG_GPIO_FUNC, 0, 0xC0))
    return false;

  // M5GFX uses 5 kHz for the PaperMono frontlight.
  const uint16_t frequency_hz = 5000;
  uint8_t freq[2] = {
      static_cast<uint8_t>(frequency_hz & 0xFF),
      static_cast<uint8_t>((frequency_hz >> 8) & 0xFF),
  };
  if (this->write_register(REG_PWM_FREQ_L, freq, sizeof(freq)) != i2c::ERROR_OK)
    return false;

  this->set_frontlight_brightness(0.0f);
  return true;
}

void M5PM1Power::setup() {
  ESP_LOGCONFIG(TAG, "Initializing M5PM1 for M5PaperMono Lite...");

  uint8_t id_buf[2] = {0, 0};
  if (this->read_register(REG_DEVICE_ID, id_buf, sizeof(id_buf)) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "M5PM1 did not respond at I2C address 0x%02X", this->address_);
    this->mark_failed();
    return;
  }

  const uint16_t device_id =
      static_cast<uint16_t>(id_buf[0]) | (static_cast<uint16_t>(id_buf[1]) << 8);
  if (device_id != M5PM1_DEVICE_ID) {
    ESP_LOGE(TAG, "Unexpected M5PM1 device ID 0x%04X (expected 0x%04X)", device_id,
             M5PM1_DEVICE_ID);
    this->mark_failed();
    return;
  }

  // M5PM1 is always-on. Explicitly disable I2C idle sleep and watchdog so a
  // previous firmware state cannot make the PMIC disappear from the bus.
  if (!this->write_u8_(REG_I2C_CFG, 0x00) || !this->write_u8_(REG_WDT_CNT, 0x00) ||
      !this->update_bits_(REG_PWR_CFG, 0, PAPERMONO_PWR_CFG_BITS)) {
    ESP_LOGE(TAG, "Failed to initialize M5PM1 power configuration");
    this->mark_failed();
    return;
  }

  if (!this->configure_frontlight_()) {
    ESP_LOGE(TAG, "Failed to configure M5PaperMono frontlight PWM");
    this->mark_failed();
    return;
  }

  delay(10);
  ESP_LOGCONFIG(TAG, "M5PM1 ready (device ID 0x%04X)", device_id);
}

void M5PM1Power::set_frontlight_brightness(float state) {
  if (state < 0.0f)
    state = 0.0f;
  if (state > 1.0f)
    state = 1.0f;

  const uint8_t brightness = static_cast<uint8_t>(state * 255.0f + 0.5f);

  if (brightness == 0) {
    // M5GFX disables PWM by clearing the high/control register.
    if (!this->write_u8_(REG_PWM0_H, 0x00))
      ESP_LOGW(TAG, "Failed to turn frontlight off");
    return;
  }

  // Match M5GFX's gamma-2 brightness curve. PWM value is encoded across
  // 0x30/0x31; bit4 of the high byte enables PWM.
  const uint32_t squared =
      static_cast<uint32_t>(brightness) * static_cast<uint32_t>(brightness);
  uint8_t pwm[2] = {
      static_cast<uint8_t>((squared >> 4) & 0xFF),
      static_cast<uint8_t>((squared >> 12) | 0x10),
  };

  if (this->write_register(REG_PWM0_L, pwm, sizeof(pwm)) != i2c::ERROR_OK)
    ESP_LOGW(TAG, "Failed to set frontlight brightness");
}

void M5PM1Power::update() {
  if (this->battery_voltage_ == nullptr)
    return;

  // M5PM1 VBAT is a little-endian 16-bit millivolt reading. Take a median of
  // five reads to reject an occasional I2C glitch during e-paper activity.
  uint16_t samples[5];
  int count = 0;
  for (int i = 0; i < 5; i++) {
    uint8_t data[2] = {0, 0};
    if (this->read_register(REG_VBAT_L, data, sizeof(data)) == i2c::ERROR_OK) {
      samples[count++] =
          (static_cast<uint16_t>(data[1]) << 8) | static_cast<uint16_t>(data[0]);
    }
  }

  if (count == 0) {
    ESP_LOGW(TAG, "VBAT read failed");
    return;
  }

  for (int i = 0; i < count; i++) {
    for (int j = i + 1; j < count; j++) {
      if (samples[j] < samples[i]) {
        const uint16_t tmp = samples[i];
        samples[i] = samples[j];
        samples[j] = tmp;
      }
    }
  }

  const uint16_t millivolts = samples[count / 2];
  if (millivolts < 3000 || millivolts > 5000) {
    ESP_LOGD(TAG, "Ignoring implausible VBAT sample: %u mV", millivolts);
    return;
  }

  this->battery_voltage_->publish_state(millivolts / 1000.0f);
}

bool M5PM1Power::is_usb_present() {
  uint8_t data[2] = {0, 0};
  if (this->read_register(REG_VIN_L, data, sizeof(data)) != i2c::ERROR_OK)
    return false;

  const uint16_t millivolts =
      (static_cast<uint16_t>(data[1]) << 8) | static_cast<uint16_t>(data[0]);
  return millivolts > 4000;
}

void M5PM1Power::power_off() {
  ESP_LOGW(TAG, "Requesting M5PM1 system shutdown");
  this->write_u8_(REG_SYS_CMD, 0xA1);
}

void M5PM1Power::dump_config() {
  ESP_LOGCONFIG(TAG, "M5PM1 Power (M5PaperMono family):");
  LOG_I2C_DEVICE(this);
  if (this->is_failed())
    ESP_LOGE(TAG, "  M5PM1 initialization failed");
}

}  // namespace m5pm1_power
}  // namespace esphome
