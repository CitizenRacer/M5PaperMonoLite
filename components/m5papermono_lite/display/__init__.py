"""PaperMono-specific SSD1677 display platform.

The M5PaperMono panel is not electrically/protocol-identical to ESPHome's
generic SSD1677 defaults. This platform keeps ESPHome's EPaperMono framebuffer,
SPI transport, rotation and update state machine, while overriding the panel's
RAM addressing and full-refresh sequence to match M5Stack's current M5GFX
PaperMono implementation.
"""

from esphome import pins
import esphome.codegen as cg
from esphome.components import display, spi
from esphome.components.epaper_spi.display import EPaperBase
import esphome.config_validation as cv
from esphome.const import (
    CONF_AUTO_CLEAR_ENABLED,
    CONF_BUSY_PIN,
    CONF_DC_PIN,
    CONF_ENABLE_PIN,
    CONF_ID,
    CONF_LAMBDA,
    CONF_PAGES,
    CONF_RESET_PIN,
    CONF_ROTATION,
)
from esphome.types import ConfigType

AUTO_LOAD = ["epaper_spi"]
DEPENDENCIES = ["spi"]

from .. import m5papermono_lite_ns

M5PaperMonoLiteDisplay = m5papermono_lite_ns.class_(
    "M5PaperMonoLiteDisplay", EPaperBase
)


def _add_display_metadata(config):
    display.add_metadata(
        config[CONF_ID],
        800,
        480,
        has_hardware_rotation=True,
        byte_order=cv.UNDEFINED,
        has_writer=config.get(CONF_AUTO_CLEAR_ENABLED) is True
        or config.get(CONF_PAGES) is not None
        or config.get(CONF_LAMBDA) is not None,
        rotation=config.get(CONF_ROTATION, 0),
        draw_rounding=0,
    )
    return config


CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(M5PaperMonoLiteDisplay),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_ENABLE_PIN): cv.ensure_list(pins.gpio_output_pin_schema),
        }
    ).extend(
        spi.spi_device_schema(
            cs_pin_required=True,
            default_mode="MODE0",
            default_data_rate=20_000_000,
        )
    ),
    _add_display_metadata,
)

FINAL_VALIDATE_SCHEMA = spi.final_validate_device_schema(
    "m5papermono_lite", require_miso=False, require_mosi=True
)


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])

    await display.register_display(var, config)
    await spi.register_spi_device(var, config, write_only=True)

    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))

    reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset))

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    if enable_pins := config.get(CONF_ENABLE_PIN):
        enable = [await cg.gpio_pin_expression(pin) for pin in enable_pins]
        cg.add(var.set_enable_pins(enable))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
