import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.components.canbus import CONF_CANBUS_ID, CanbusComponent
from esphome.const import CONF_ID

DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["rego1000", "binary_sensor"]

rego_ns = cg.esphome_ns.namespace("rego")
RegoSensor = rego_ns.class_("RegoBinarySensor", binary_sensor.BinarySensor, cg.Component)

CONFIG_SCHEMA = binary_sensor.BINARY_SENSOR_SCHEMA.extend( 
    {
        cv.GenerateID(): cv.declare_id(RegoSensor),
        cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
        cv.Exclusive("rego_variable", "rego_can_id"): cv.int_,
        cv.Exclusive("rego_listen_can_id", "rego_can_id"): cv.int_,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    await binary_sensor.register_binary_sensor(var, config)
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    if "rego_variable" in config:
        cg.add(var.set_rego_variable(int(config["rego_variable"])))
    if "rego_listen_can_id" in config:
        cg.add(var.set_rego_listen_can_id(int(config["rego_listen_can_id"])))
