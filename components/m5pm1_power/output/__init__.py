import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import output
from esphome.const import CONF_ID

from .. import BASE_SCHEMA, m5pm1_power_ns

DEPENDENCIES = ["m5pm1_power"]

M5PM1FrontlightOutput = m5pm1_power_ns.class_(
    "M5PM1FrontlightOutput", output.FloatOutput, cg.Component
)

CONFIG_SCHEMA = (
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {cv.Required(CONF_ID): cv.declare_id(M5PM1FrontlightOutput)}
    )
    .extend(BASE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_parented(var, config["m5pm1_power_id"])
    await cg.register_component(var, config)
    await output.register_output(var, config)
