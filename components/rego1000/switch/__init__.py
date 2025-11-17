import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.components.canbus import CONF_CANBUS_ID, CanbusComponent
from esphome.const import CONF_ID

DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["rego1000", "switch"]

rego_ns = cg.esphome_ns.namespace("rego")
RegoSwitch = rego_ns.class_("RegoSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = (
    switch.switch_schema(RegoSwitch)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(RegoSwitch),
            cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
            cv.Required("rego_variable"): cv.int_,
            cv.Optional("poll_interval", default="10s"): cv.update_interval,
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    await switch.register_switch(var, config)
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    cg.add(var.set_rego_variable(int(config["rego_variable"])))
    cg.add(var.set_poll_interval(config["poll_interval"]))
