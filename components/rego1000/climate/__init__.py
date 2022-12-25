import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate
from esphome.components.sensor import Sensor
from esphome.components.canbus import CONF_CANBUS_ID, CanbusComponent
from esphome.const import CONF_ID, CONF_SENSOR_ID

DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["rego1000", "climate"]

rego_ns = cg.esphome_ns.namespace("rego")
RegoNumber = rego_ns.class_("RegoClimate", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend( 
    {
        cv.GenerateID(): cv.declare_id(RegoNumber),
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Required("rego_setpoint_variable"): cv.int_,
        cv.Optional(CONF_SENSOR_ID): cv.use_id(Sensor),
        cv.Optional("value_factor", default=1.): cv.float_,
        cv.Optional("poll_interval", default="10s"): cv.update_interval,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    await climate.register_climate(var, config)
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    cg.add(var.set_rego_variable(int(config["rego_setpoint_variable"])))
    cg.add(var.set_value_factor(config["value_factor"]))
    cg.add(var.set_poll_interval(config["poll_interval"]))
    if CONF_SENSOR_ID in config:
        indoor_sensor = await cg.get_variable(config[CONF_SENSOR_ID])
        cg.add(var.set_indoor_sensor(indoor_sensor))

