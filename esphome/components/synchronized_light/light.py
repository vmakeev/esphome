import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_LIGHT_ID,
    CONF_OUTPUT_ID,
)
import esphome.final_validate as fv

CONF_PRIMARY_LIGHT = "primary_light"
CONF_LINKED_LIGHTS = "linked_lights"

synchronized_light_ns = cg.esphome_ns.namespace("synchronized_light")
SynchronizedLinkedLight = synchronized_light_ns.class_("SynchronizedLinkedLight")
SynchronizedPrimaryLight = synchronized_light_ns.class_("SynchronizedPrimaryLight")
SynchronizedLightOutput = synchronized_light_ns.class_(
    "SynchronizedLightOutput", light.AddressableLight
)

PRIMARY_LIGHT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(SynchronizedPrimaryLight),
        cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState),
    }
)

LINKED_LIGHT_SCHEMA = cv.Schema(
    {cv.Required(CONF_LIGHT_ID): cv.use_id(light.LightState)}
)

CONFIG_SCHEMA = light.ADDRESSABLE_LIGHT_SCHEMA.extend(
    {
        cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(SynchronizedLightOutput),
        cv.Required(CONF_PRIMARY_LIGHT): PRIMARY_LIGHT_SCHEMA,
        cv.Required(CONF_LINKED_LIGHTS): cv.All(
            cv.ensure_list(cv.Any(LINKED_LIGHT_SCHEMA)),
            cv.Length(min=1),
        ),
    }
)


async def to_code(config):
    linked_lights = []
    for conf in config[CONF_LINKED_LIGHTS]:
        linked_lights.append(
            SynchronizedLinkedLight(await cg.get_variable(conf[CONF_LIGHT_ID]))
        )

    primary_light = cg.new_Pvariable(
        config[CONF_PRIMARY_LIGHT][CONF_ID],
        SynchronizedPrimaryLight(
            await cg.get_variable(config[CONF_PRIMARY_LIGHT][CONF_LIGHT_ID])
        ),
    )

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID], primary_light, linked_lights)

    await cg.register_component(var, config)
    await light.register_light(var, config)
