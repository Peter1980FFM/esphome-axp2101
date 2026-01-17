# Minimal stub so ESPHome recognizes this component.
# Do NOT import XPowersLib or old AXP2101 classes.

from esphome.const import CONF_ID
import esphome.config_validation as cv
import esphome.codegen as cg

axp2101_ns = cg.esphome_ns.namespace("axp2101")
AXP2101Component = axp2101_ns.class_("AXP2101Component", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(AXP2101Component),
})

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
