"""M5PM1 power management for the M5Stack M5PaperMono family.

This started from PFalko's MIT-licensed m5pm1_power component for PaperColor,
then adapts the initialization to M5Stack's current M5PaperMono implementation.
The PaperMono e-paper rail is controlled by M5IOE1, not by M5PM1 GPIO0.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_BATTERY_VOLTAGE,
    CONF_ID,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
)

AUTO_LOAD = ["sensor"]
CODEOWNERS = ["@CitizenRacer"]
DEPENDENCIES = ["i2c"]

m5pm1_power_ns = cg.esphome_ns.namespace("m5pm1_power")
M5PM1Power = m5pm1_power_ns.class_(
    "M5PM1Power", cg.PollingComponent, i2c.I2CDevice
)

CONF_M5PM1_POWER_ID = "m5pm1_power_id"

BASE_SCHEMA = cv.Schema(
    {cv.GenerateID(CONF_M5PM1_POWER_ID): cv.use_id(M5PM1Power)}
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(M5PM1Power),
            cv.Optional(CONF_BATTERY_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x6E))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_BATTERY_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_VOLTAGE])
        cg.add(var.set_battery_voltage_sensor(sens))
