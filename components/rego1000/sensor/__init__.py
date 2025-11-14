import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.components.canbus import CONF_CANBUS_ID, CanbusComponent
from esphome.const import CONF_ID

DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["rego1000", "sensor"]

rego_ns = cg.esphome_ns.namespace("rego")
RegoSensor = rego_ns.class_("RegoSensor", sensor.Sensor, cg.Component)

CONFIG_SCHEMA = (
    sensor.sensor_schema(RegoSensor)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(RegoSensor),
            cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
            cv.Exclusive("rego_variable", "rego_can_id"): cv.int_,
            cv.Exclusive("rego_listen_can_id", "rego_can_id"): cv.int_,
            cv.Optional("value_factor", default=1.): cv.float_,
            cv.Optional("poll_interval", default="10s"): cv.update_interval,
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    await sensor.register_sensor(var, config)
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    if "rego_variable" in config:
        cg.add(var.set_rego_variable(int(config["rego_variable"])))
    if "rego_listen_can_id" in config:
        cg.add(var.set_rego_listen_can_id(int(config["rego_listen_can_id"])))
    cg.add(var.set_value_factor(config["value_factor"]))
    cg.add(var.set_poll_interval(config["poll_interval"]))
