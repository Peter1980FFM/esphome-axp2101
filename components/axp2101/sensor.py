import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["i2c"]

axp2101_ns = cg.esphome_ns.namespace("axp2101")
AXP2101Component = axp2101_ns.class_("AXP2101Component", cg.PollingComponent, i2c.I2CDevice)

CONF_MODEL = "model"
CONF_BRIGHTNESS = "brightness"
CONF_BATTERY_CHARGING = "battery_charging"
CONF_VBUS_VOLTAGE = "vbus_voltage"
CONF_VBUS_CURRENT = "vbus_current"
CONF_TEMPERATURE = "temperature"
CONF_APS_VOLTAGE = "aps_voltage"
CONF_POWER_KEY = "power_key"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AXP2101Component),

    cv.Required(CONF_MODEL): cv.string,
    cv.Optional("address", default=0x34): cv.i2c_address,
    cv.Optional("update_interval", default="30s"): cv.update_interval,
    cv.Optional(CONF_BRIGHTNESS, default=128): cv.int_range(min=0, max=255),

    cv.Optional(CONF_BATTERY_CHARGING): sensor.sensor_schema(),
    cv.Optional(CONF_VBUS_VOLTAGE): sensor.sensor_schema(),
    cv.Optional(CONF_VBUS_CURRENT): sensor.sensor_schema(),
    cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(),
    cv.Optional(CONF_APS_VOLTAGE): sensor.sensor_schema(),
    cv.Optional(CONF_POWER_KEY): text_sensor.text_sensor_schema(),
}).extend(i2c.i2c_device_schema(0x34)).extend(cv.polling_component_schema("30s"))


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield i2c.register_i2c_device(var, config)

    cg.add(var.set_brightness(config[CONF_BRIGHTNESS]))

    if CONF_BATTERY_CHARGING in config:
        sens = yield sensor.new_sensor(config[CONF_BATTERY_CHARGING])
        cg.add(var.battery_charging_sensor.set(sens))

    if CONF_VBUS_VOLTAGE in config:
        sens = yield sensor.new_sensor(config[CONF_VBUS_VOLTAGE])
        cg.add(var.vbus_voltage_sensor.set(sens))

    if CONF_VBUS_CURRENT in config:
        sens = yield sensor.new_sensor(config[CONF_VBUS_CURRENT])
        cg.add(var.vbus_current_sensor.set(sens))

    if CONF_TEMPERATURE in config:
        sens = yield sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.temperature_sensor.set(sens))

    if CONF_APS_VOLTAGE in config:
        sens = yield sensor.new_sensor(config[CONF_APS_VOLTAGE])
        cg.add(var.aps_voltage_sensor.set(sens))

    if CONF_POWER_KEY in config:
        sens = yield text_sensor.new_text_sensor(config[CONF_POWER_KEY])
        cg.add(var.power_key_sensor.set(sens))
