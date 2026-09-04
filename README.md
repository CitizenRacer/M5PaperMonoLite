# M5PaperMonoLite ESPHome support

ESPHome hardware support for the **M5Stack M5PaperMono Lite (C153-LITE)**,
built from the architecture of
[PFalko/m5stack-papercolor-esphome](https://github.com/PFalko/m5stack-papercolor-esphome)
and adapted to the PaperMono hardware using M5Stack's current M5GFX,
M5Unified, M5IOE1 and PaperMono reference implementations.

## Status

Initial bring-up support is implemented for the parts needed to use the device
as an ESPHome touchscreen e-paper controller:

- ESP32-S3, 16 MB flash, 8 MB octal PSRAM
- SSD1677 800x480 e-paper through ESPHome `epaper_spi`
- M5IOE1-controlled e-paper power and reset
- FT6336G touch through ESPHome `ft5x06`
- M5IOE1-controlled touch power and reset
- M5PM1 initialization
- frontlight brightness as a normal ESPHome monochromatic light
- battery voltage telemetry
- GitHub Actions compile validation

The implementation intentionally uses ESPHome's native SSD1677 and FT5x06
drivers rather than maintaining private display/touch drivers.

### Current limitation: grayscale

The panel supports four gray levels, but ESPHome's current SSD1677
`EPaperMono` implementation is 1-bit. This package therefore renders
black/white. M5Stack's official OTP demo and M5GFX contain the reference
behavior needed for a future four-gray ESPHome enhancement.

## Hardware mapping

| Function | PaperMono Lite connection |
| --- | --- |
| Internal I2C SDA | GPIO47 |
| Internal I2C SCL | GPIO48 |
| EPD MOSI | GPIO14 |
| EPD CLK | GPIO15 |
| EPD CS | GPIO16 |
| EPD DC | GPIO17 |
| EPD BUSY | GPIO18 |
| EPD power | M5IOE1 physical PIN3 / ESPHome number 2 |
| EPD reset | M5IOE1 physical PIN5 / ESPHome number 4 |
| Touch interrupt | GPIO4 |
| Touch power | M5IOE1 physical PIN13 / ESPHome number 12 |
| Touch reset | M5IOE1 physical PIN6 / ESPHome number 5 |
| Frontlight | M5PM1 GPIO3 PWM |
| M5IOE1 address | `0x4F` |
| M5PM1 address | `0x6E` |
| FT6336G address | `0x38` |

M5Stack's M5IOE1 API names its pins from 1, while the ESPHome M5IOE1
component uses zero-based pin numbers. The package performs that translation
explicitly.

## Use it

Import the hardware package into an ESPHome device:

```yaml
esphome:
  name: papermono-lite
  friendly_name: PaperMono Lite

packages:
  m5papermono_lite:
    url: https://github.com/CitizenRacer/M5PaperMonoLite
    ref: main
    file: packages/m5papermono-lite.yaml
    refresh: 1d

logger:

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:

ota:
  - platform: esphome
```

The package exposes these useful IDs:

| ID | Type |
| --- | --- |
| `m5_display` | ESPHome display |
| `m5_touch` | touchscreen |
| `m5_frontlight` | light |
| `m5_frontlight_output` | float output |
| `m5_battery_voltage` | sensor |
| `m5_pmic` | M5PM1 component |
| `m5_ioe` | M5IOE1 hub |

The display defaults to portrait orientation (logical 480x800). Override the
package's `m5_display` item by ID in your device YAML when you add a display
lambda/pages or change update behavior.

## Why there are two small custom components

`components/m5pm1_power` is derived from PFalko's PaperColor component but
uses the **PaperMono** PMIC sequence. PaperColor routes e-paper power through
M5PM1 GPIO0; PaperMono does not. On PaperMono, e-paper power/reset are on the
M5IOE1. The PMIC component therefore limits itself to PMIC setup, frontlight,
battery telemetry, and system shutdown support.

`components/m5papermono_lite` handles the sequencing ESPHome's FT5x06 driver
cannot express: assert touch power through M5IOE1, pulse touch reset, then let
the stock touchscreen driver initialize normally.

Keeping these responsibilities separate also preserves startup ordering:

1. ESPHome I2C bus starts.
2. M5PM1 is configured.
3. M5IOE1 starts.
4. touch power/reset is sequenced.
5. native ESPHome display and touchscreen drivers initialize.

## Source references

The implementation was checked against:

- PFalko's `m5stack-papercolor-esphome` project (MIT)
- M5Stack `M5GFX` PaperMono board support (MIT)
- M5Stack `M5Unified` PaperMono board support (MIT)
- M5Stack `M5PaperMono-OTP-Demo` (MIT)
- M5Stack `M5PaperMono-UserDemo` (MIT)
- M5Stack's ESPHome `m5ioe1` external component
- current ESPHome `epaper_spi` SSD1677 and `ft5x06` implementations

See [`NOTICE`](NOTICE) for attribution.

## Next hardware-validation steps

The current code is intended to compile before hardware arrives and to make
first bring-up safe and observable. On real C153-LITE hardware, validate in
this order:

1. boot log finds M5PM1 device ID `0x2050` and M5IOE1 `0x4F`;
2. frontlight turns on/off and dims correctly;
3. EPD performs a clean black/white full refresh;
4. touch coordinates match the 480x800 portrait display;
5. battery voltage is plausible on USB and battery;
6. exercise partial refresh and select an appropriate `full_update_every`;
7. add PMIC power-button/shutdown/wake behavior;
8. optionally add RTC, IMU and PDM microphone support.

## License

MIT. See [`LICENSE`](LICENSE). PFalko-derived portions retain attribution in
source and in [`NOTICE`](NOTICE).
