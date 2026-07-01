import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID

CODEOWNERS = ["@stackdrift"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor", "binary_sensor"]
MULTI_CONF = True

CONF_M5_THERMAL2_ID = "m5_thermal2_id"
CONF_REFRESH_RATE = "refresh_rate"
CONF_NOISE_FILTER = "noise_filter"
CONF_MONITOR_AREA = "monitor_area"
CONF_WIDTH = "width"
CONF_HEIGHT = "height"

m5_thermal2_ns = cg.esphome_ns.namespace("m5_thermal2")
M5Thermal2 = m5_thermal2_ns.class_("M5Thermal2", cg.PollingComponent, i2c.I2CDevice)

# string -> unit refresh_rate register code (0x6E / config[3])
REFRESH_RATES = {
    "0.5Hz": 0,
    "1Hz": 1,
    "2Hz": 2,
    "4Hz": 3,
    "8Hz": 4,
    "16Hz": 5,
    "32Hz": 6,
    "64Hz": 7,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(M5Thermal2),
            cv.Optional(CONF_REFRESH_RATE, default="16Hz"): cv.enum(
                REFRESH_RATES, upper=False
            ),
            cv.Optional(CONF_NOISE_FILTER, default=8): cv.int_range(min=0, max=15),
            cv.Optional(CONF_MONITOR_AREA): cv.Schema(
                {
                    cv.Optional(CONF_WIDTH, default=15): cv.int_range(min=0, max=15),
                    cv.Optional(CONF_HEIGHT, default=11): cv.int_range(min=0, max=11),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x32))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_refresh_rate(config[CONF_REFRESH_RATE]))
    cg.add(var.set_noise_filter(config[CONF_NOISE_FILTER]))
    if CONF_MONITOR_AREA in config:
        area = config[CONF_MONITOR_AREA]
        cg.add(var.set_monitor_area(area[CONF_WIDTH], area[CONF_HEIGHT]))
