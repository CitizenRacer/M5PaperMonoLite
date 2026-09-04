# M5PaperMonoLite ESPHome support

ESPHome hardware support for the **M5Stack M5PaperMono Lite (C153-LITE)**.
The project uses
[PFalko/m5stack-papercolor-esphome](https://github.com/PFalko/m5stack-papercolor-esphome)
as its architectural foundation, then adapts the power and display handling to
the PaperMono hardware using M5Stack's current M5GFX, M5Unified, M5IOE1 and
PaperMono reference implementations.

## Status

Initial bring-up support is implemented for the parts needed to use the device
as an ESPHome touchscreen e-paper controller:

- ESP32-S3, 16 MB flash, 8 MB octal PSRAM
- PaperMono-specific SSD1677 800x480 monochrome display driver
- M5IOE1-controlled e-paper power and reset
- FT6336G touch through ESPHome's `ft5x06` driver
- M5IOE1-controlled touch power and reset
- M5PM1 initialization
- frontlight brightness as a normal ESPHome monochromatic light
- battery-voltage telemetry
- GitHub Actions configuration/compile workflow

The custom display platform inherits ESPHome's `EPaperMono` framebuffer, SPI
transport, rotation handling, and update state machine. It overrides only the
parts that are different on PaperMono: reversed gate/Y addressing, two-plane
full-frame transfer, full-refresh control, analog power-down, and deep sleep.

### Deliberate first-bring-up limitations

- **Monochrome only.** The panel supports four gray levels, but ESPHome's
  current SSD1677 framebuffer is 1-bit. M5Stack's OTP demo and M5GFX provide
  the reference behavior for a future four-gray implementation.
- **Full refresh only.** M5Stack supports fast/partial refresh, but partial
  refresh depends on retained SSD1677 RAM and different reset/waveform rules.
  It is intentionally not enabled until the full-refresh path has been
  validated on real C153-LITE hardware.

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

M5Stack names M5IOE1 pins from 1, while the ESPHome M5IOE1 component uses
zero-based pin numbers. The package performs that translation explicitly.

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
| `m5_display` | PaperMono display |
| `m5_touch` | touchscreen |
| `m5_frontlight` | light |
| `m5_frontlight_output` | float output |
| `m5_battery_voltage` | sensor |
| `m5_pmic` | M5PM1 component |
| `m5_ioe` | M5IOE1 hub |

The display defaults to portrait orientation (logical 480x800). Add a display
lambda/pages by merging the `m5_display` item by ID in your device YAML.

## Architecture

### `components/m5pm1_power`

Derived from PFalko's MIT-licensed PaperColor component, but with the power
sequence corrected for PaperMono. PaperColor routes e-paper power through
M5PM1 GPIO0; PaperMono routes e-paper power/reset through M5IOE1. On
PaperMono, M5PM1 is responsible here for PMIC setup, the GPIO3 frontlight PWM,
battery telemetry, and system shutdown support.

The frontlight follows M5Stack's current implementation: 5 kHz PWM with the
same gamma-squared brightness mapping used by M5GFX.

### `components/m5papermono_lite`

The board component handles the touch sequencing ESPHome's FT5x06 component
cannot express: assert touch power through M5IOE1, pulse touch reset, then let
the stock FT5x06 driver initialize.

Its `display` platform subclasses ESPHome's `EPaperMono`. This is necessary
because the generic ESPHome SSD1677 path is not sufficient for this panel:
M5Stack's current PaperMono driver reverses the gate/Y direction and uses a
different full-refresh/two-RAM-plane sequence.

Startup ordering is intentional:

1. ESPHome I2C bus starts.
2. M5PM1 is configured.
3. M5IOE1 starts.
4. touch power/reset is sequenced.
5. PaperMono display and FT6336G touchscreen initialize.

## Source references

Implementation was checked against:

- PFalko `m5stack-papercolor-esphome` (MIT)
- M5Stack `M5GFX` PaperMono support
- M5Stack `M5Unified` PaperMono support
- M5Stack `M5PaperMono-OTP-Demo`
- M5Stack `M5PaperMono-UserDemo`
- M5Stack's ESPHome `m5ioe1` component
- current ESPHome `epaper_spi` / `EPaperMono` and `ft5x06` implementations

See [`NOTICE`](NOTICE) for attribution.

## Validation plan

Before calling the hardware support production-ready, validate on a real
C153-LITE in this order:

1. boot log finds M5PM1 device ID `0x2050` and M5IOE1 at `0x4F`;
2. frontlight turns on/off and dims correctly;
3. EPD produces a clean, correctly oriented black/white full refresh;
4. touch coordinates match the 480x800 portrait display;
5. battery voltage is plausible on USB and battery;
6. validate analog power-down and repeated wake/full-refresh cycles;
7. implement and validate retained-RAM partial refresh;
8. add four-gray mode if useful;
9. optionally add RTC, IMU, microphone and PMIC power-button/wake features.

## Build validation

`.github/workflows/compile.yml` runs `esphome config` and `esphome compile`
against `examples/minimal.yaml` on normal GitHub pushes and pull requests.

## License

MIT. See [`LICENSE`](LICENSE). PFalko-derived portions retain attribution in
source and in [`NOTICE`](NOTICE).
