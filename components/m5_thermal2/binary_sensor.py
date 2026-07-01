import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor

from . import CONF_M5_THERMAL2_ID, M5Thermal2

DEPENDENCIES = ["m5_thermal2"]

CONF_BUTTON = "button"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_M5_THERMAL2_ID): cv.use_id(M5Thermal2),
        cv.Optional(CONF_BUTTON): binary_sensor.binary_sensor_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_M5_THERMAL2_ID])
    if CONF_BUTTON in config:
        b = await binary_sensor.new_binary_sensor(config[CONF_BUTTON])
        cg.add(parent.set_button_sensor(b))
