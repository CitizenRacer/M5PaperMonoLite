"""Board-level GPIO sequencing for M5Stack M5PaperMono Lite."""

from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@CitizenRacer"]

m5papermono_lite_ns = cg.esphome_ns.namespace("m5papermono_lite")
M5PaperMonoLite = m5papermono_lite_ns.class_("M5PaperMonoLite", cg.Component)

CONF_TOUCH_POWER_PIN = "touch_power_pin"
CONF_TOUCH_RESET_PIN = "touch_reset_pin"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(M5PaperMonoLite),
        cv.Required(CONF_TOUCH_POWER_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_TOUCH_RESET_PIN): pins.gpio_output_pin_schema,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    touch_power = await cg.gpio_pin_expression(config[CONF_TOUCH_POWER_PIN])
    cg.add(var.set_touch_power_pin(touch_power))

    touch_reset = await cg.gpio_pin_expression(config[CONF_TOUCH_RESET_PIN])
    cg.add(var.set_touch_reset_pin(touch_reset))
