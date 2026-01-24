import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, sensor, number
from esphome.const import CONF_ID

CONF_ROOM_SENSOR = "room_sensor"
CONF_REAL_AC = "real_ac"

CONF_HOME_MIN = "home_min"
CONF_HOME_MAX = "home_max"
CONF_SLEEP_MIN = "sleep_min"
CONF_SLEEP_MAX = "sleep_max"
CONF_AWAY_MIN = "away_min"
CONF_AWAY_MAX = "away_max"

virtual_thermostat_ns = cg.esphome_ns.namespace("virtual_thermostat")
VirtualThermostat = virtual_thermostat_ns.class_("VirtualThermostat", climate.Climate, cg.Component)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(VirtualThermostat),

        cv.Required(CONF_ROOM_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_REAL_AC): cv.use_id(climate.Climate),

        cv.Required(CONF_HOME_MIN): cv.use_id(number.Number),
        cv.Required(CONF_HOME_MAX): cv.use_id(number.Number),
        cv.Required(CONF_SLEEP_MIN): cv.use_id(number.Number),
        cv.Required(CONF_SLEEP_MAX): cv.use_id(number.Number),
        cv.Required(CONF_AWAY_MIN): cv.use_id(number.Number),
        cv.Required(CONF_AWAY_MAX): cv.use_id(number.Number),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)

    room = await cg.get_variable(config[CONF_ROOM_SENSOR])
    cg.add(var.room_sensor = room)

    real = await cg.get_variable(config[CONF_REAL_AC])
    cg.add(var.real_ac = real)

    home_min = await cg.get_variable(config[CONF_HOME_MIN])
    home_max = await cg.get_variable(config[CONF_HOME_MAX])
    sleep_min = await cg.get_variable(config[CONF_SLEEP_MIN])
    sleep_max = await cg.get_variable(config[CONF_SLEEP_MAX])
    away_min = await cg.get_variable(config[CONF_AWAY_MIN])
    away_max = await cg.get_variable(config[CONF_AWAY_MAX])

    cg.add(var.home.min_entity = home_min)
    cg.add(var.home.max_entity = home_max)

    cg.add(var.sleep.min_entity = sleep_min)
    cg.add(var.sleep.max_entity = sleep_max)

    cg.add(var.away.min_entity = away_min)
    cg.add(var.away.max_entity = away_max)

