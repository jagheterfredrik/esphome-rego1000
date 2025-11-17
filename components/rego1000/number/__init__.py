import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.components.canbus import CONF_CANBUS_ID, CanbusComponent
from esphome.const import CONF_ID, CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_STEP

DEPENDENCIES = ["canbus"]
AUTO_LOAD = ["rego1000", "number"]

rego_ns = cg.esphome_ns.namespace("rego")
RegoNumber = rego_ns.class_("RegoNumber", number.Number, cg.Component)

CONFIG_SCHEMA = (
    number.number_schema(RegoNumber)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(RegoNumber),
            cv.GenerateID(CONF_CANBUS_ID): cv.use_id(CanbusComponent),
            cv.Required("rego_variable"): cv.int_,
            cv.Required(CONF_MAX_VALUE): cv.float_,
            cv.Required(CONF_MIN_VALUE): cv.float_,
            cv.Required(CONF_STEP): cv.positive_float,
            cv.Optional("value_factor", default=1.): cv.float_,
            cv.Optional("poll_interval", default="10s"): cv.update_interval,
        }
    )
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
        
    await number.register_number(var, config, min_value=config[CONF_MIN_VALUE], max_value=config[CONF_MAX_VALUE], step=config[CONF_STEP])
    canbus_component = await cg.get_variable(config[CONF_CANBUS_ID])
    cg.add(var.set_canbus(canbus_component))
    cg.add(var.set_rego_variable(int(config["rego_variable"])))
    cg.add(var.set_value_factor(config.get("value_factor")))
    cg.add(var.set_poll_interval(config["poll_interval"]))
