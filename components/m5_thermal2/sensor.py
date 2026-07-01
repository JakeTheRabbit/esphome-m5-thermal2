import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from . import CONF_M5_THERMAL2_ID, M5Thermal2

DEPENDENCIES = ["m5_thermal2"]

CONF_AVERAGE_TEMPERATURE = "average_temperature"
CONF_MEDIAN_TEMPERATURE = "median_temperature"
CONF_MAX_TEMPERATURE = "max_temperature"
CONF_MIN_TEMPERATURE = "min_temperature"
CONF_DIFF_TEMPERATURE = "diff_temperature"
CONF_HOTSPOT_X = "hotspot_x"
CONF_HOTSPOT_Y = "hotspot_y"

# key -> C++ setter name
SETTERS = {
    CONF_AVERAGE_TEMPERATURE: "set_average_sensor",
    CONF_MEDIAN_TEMPERATURE: "set_median_sensor",
    CONF_MAX_TEMPERATURE: "set_max_sensor",
    CONF_MIN_TEMPERATURE: "set_min_sensor",
    CONF_DIFF_TEMPERATURE: "set_diff_sensor",
    CONF_HOTSPOT_X: "set_hotspot_x_sensor",
    CONF_HOTSPOT_Y: "set_hotspot_y_sensor",
}


def _temperature_schema():
    return sensor.sensor_schema(
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


def _coord_schema():
    return sensor.sensor_schema(
        accuracy_decimals=0,
        icon="mdi:crosshairs-gps",
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_M5_THERMAL2_ID): cv.use_id(M5Thermal2),
        cv.Optional(CONF_AVERAGE_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_MEDIAN_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_MAX_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_MIN_TEMPERATURE): _temperature_schema(),
        cv.Optional(CONF_DIFF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HOTSPOT_X): _coord_schema(),
        cv.Optional(CONF_HOTSPOT_Y): _coord_schema(),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_M5_THERMAL2_ID])
    for key, setter in SETTERS.items():
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(parent, setter)(sens))
