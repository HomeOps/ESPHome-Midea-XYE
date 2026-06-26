from pathlib import Path
import subprocess

from esphome.core import coroutine
from esphome import automation
from esphome.components import binary_sensor, climate, sensor, uart, remote_transmitter, number
from esphome.components.remote_base import CONF_TRANSMITTER_ID
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_AUTOCONF,
    CONF_BEEPER,
    CONF_CUSTOM_FAN_MODES,
    CONF_CUSTOM_PRESETS,
    CONF_ID,
    CONF_PERIOD,
    CONF_SUPPORTED_MODES,
    CONF_SUPPORTED_PRESETS,
    CONF_SUPPORTED_SWING_MODES,
    CONF_TIMEOUT,
    CONF_TEMPERATURE,
    CONF_USE_FAHRENHEIT,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_ICON,
    CONF_MODE,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_EMPTY,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_POWER,
    ICON_THERMOMETER,
    ICON_WATER_PERCENT,
    ICON_TIMER,
    ICON_BUG,
    ICON_SECURITY,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_WATT,
    UNIT_AMPERE,
    UNIT_MINUTE,
    UNIT_EMPTY,
)
from esphome.components.climate import (
    ClimateMode,
    ClimatePreset,
    ClimateSwingMode,
)

#CODEOWNERS = ["@dudanov"]
DEPENDENCIES = ["climate", "uart", "wifi"]
AUTO_LOAD = ["binary_sensor", "number", "sensor"]
CONF_OUTDOOR_TEMPERATURE = "outdoor_temperature"
CONF_TEMPERATURE_2A = "temperature_2a"
CONF_TEMPERATURE_2B = "temperature_2b"
CONF_TEMPERATURE_3 = "temperature_3"
CONF_CURRENT = "current"
CONF_TIMER_START = "timer_start"
CONF_TIMER_STOP = "timer_stop"
CONF_ERROR_FLAGS = "error_flags"
CONF_PROTECT_FLAGS = "protect_flags"
CONF_POWER_USAGE = "power_usage"
CONF_HUMIDITY_SETPOINT = "humidity_setpoint"
CONF_STATIC_PRESSURE = "static_pressure"
CONF_FOLLOW_ME_SENSOR = "follow_me_sensor"
CONF_INTERNAL_CURRENT_TEMPERATURE = "internal_current_temperature"
CONF_INTERNAL_TARGET_TEMPERATURE = "internal_target_temperature"
CONF_DEFROST = "defrost"
CONF_COMPRESSOR_ACTIVE = "compressor_active"
CONF_FAN_SPEED = "fan_speed"
CONF_COMPRESSOR_AWARE_ACTION = "compressor_aware_action"
CONF_SYNC_FAN_MODE_FROM_DEVICE = "sync_fan_mode_from_device"
CONF_TARGET_TEMPERATURE_ENCODING = "target_temperature_encoding"
CONF_SENSOR_TEMPERATURE_ENCODING = "sensor_temperature_encoding"
CONF_TARGET_TEMPERATURE_SOURCE = "target_temperature_source"
midea_xye_ns = cg.esphome_ns.namespace("midea").namespace("xye")
ClimateMideaXYE = midea_xye_ns.class_("ClimateMideaXYE", climate.Climate, cg.Component)
StaticPressureNumber = midea_xye_ns.class_("StaticPressureNumber", number.Number, cg.Component)
Capabilities = midea_xye_ns.namespace("Constants")

def templatize(value):
    if isinstance(value, cv.Schema):
        value = value.schema
    ret = {}
    for key, val in value.items():
        ret[key] = cv.templatable(val)
    return cv.Schema(ret)


def register_action(name, type_, schema):
    validator = templatize(schema).extend(MIDEA_ACTION_BASE_SCHEMA)
    registerer = automation.register_action(f"midea_xye.{name}", type_, validator, synchronous=True)

    def decorator(func):
        async def new_func(config, action_id, template_arg, args):
            ac_ = await cg.get_variable(config[CONF_ID])
            var = cg.new_Pvariable(action_id, template_arg)
            cg.add(var.set_parent(ac_))
            await coroutine(func)(var, config, args)
            return var

        return registerer(new_func)

    return decorator


ALLOWED_CLIMATE_MODES = {
    "HEAT_COOL": ClimateMode.CLIMATE_MODE_HEAT_COOL,
    "COOL": ClimateMode.CLIMATE_MODE_COOL,
    "HEAT": ClimateMode.CLIMATE_MODE_HEAT,
    "DRY": ClimateMode.CLIMATE_MODE_DRY,
    "FAN_ONLY": ClimateMode.CLIMATE_MODE_FAN_ONLY,
}

ALLOWED_CLIMATE_PRESETS = {
    "ECO": ClimatePreset.CLIMATE_PRESET_ECO,
    "BOOST": ClimatePreset.CLIMATE_PRESET_BOOST,
    "SLEEP": ClimatePreset.CLIMATE_PRESET_SLEEP,
}

ALLOWED_CLIMATE_SWING_MODES = {
    "BOTH": ClimateSwingMode.CLIMATE_SWING_BOTH,
    "VERTICAL": ClimateSwingMode.CLIMATE_SWING_VERTICAL,
    "HORIZONTAL": ClimateSwingMode.CLIMATE_SWING_HORIZONTAL,
}

CUSTOM_FAN_MODES = {
    "SILENT": Capabilities.SILENT,
    "TURBO": Capabilities.TURBO,
}

CUSTOM_PRESETS = {
    "FREEZE_PROTECTION": Capabilities.FREEZE_PROTECTION,
}

TEMPERATURE_ENCODINGS = {
    "STANDARD": "standard",
    "RAW_FAHRENHEIT": "raw_fahrenheit",
}

TARGET_TEMPERATURE_SOURCES = {
    "C4": "c4",
    "C0": "c0",
}

SOURCE_DIR = Path(__file__).resolve().parent


def _run_git(args):
    try:
        return subprocess.check_output(
            ["git", "-C", str(SOURCE_DIR), *args],
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError, OSError):
        return ""


def _git_build_info():
    commit = _run_git(["rev-parse", "HEAD"]) or "unknown"
    branch = _run_git(["branch", "--show-current"])
    if not branch:
        branch = _run_git(["describe", "--all", "--exact-match", "HEAD"])
    if not branch:
        branch = _run_git(["name-rev", "--name-only", "--no-undefined", "HEAD"])
    if not branch:
        branch = "detached"
    remote = _run_git(["remote", "get-url", "origin"]) or "unknown"
    dirty = 1 if _run_git(["status", "--porcelain"]) else 0
    return {
        "commit": commit,
        "branch": branch,
        "remote": remote,
        "dirty": dirty,
    }

def _config_candidates(value):
    yield value
    for attr in ("value", "name"):
        attr_value = getattr(value, attr, None)
        if attr_value is not None:
            yield attr_value
    yield str(value)
    yield repr(value)


def _normalize_config_candidate(value):
    return str(value).strip().strip("'\"").lower().replace("-", "_")


def _config_matches(value, expected):
    expected = expected.lower()
    for candidate in _config_candidates(value):
        normalized = _normalize_config_candidate(candidate)
        if normalized == expected or normalized.endswith(f".{expected}") or expected in normalized:
            return True
    return False


def _add_git_build_defines(config):
    info = _git_build_info()
    raw_fahrenheit_target_temperatures = _config_matches(config[CONF_TARGET_TEMPERATURE_ENCODING], "raw_fahrenheit")
    raw_fahrenheit_sensor_temperatures = _config_matches(config[CONF_SENSOR_TEMPERATURE_ENCODING], "raw_fahrenheit")
    target_temperature_from_c0 = _config_matches(config[CONF_TARGET_TEMPERATURE_SOURCE], "c0")
    cg.add_define("MIDEA_XYE_BUILD_GIT_COMMIT", info["commit"])
    cg.add_define("MIDEA_XYE_BUILD_GIT_BRANCH", info["branch"])
    cg.add_define("MIDEA_XYE_BUILD_GIT_REMOTE", info["remote"])
    cg.add_define("MIDEA_XYE_BUILD_GIT_DIRTY", info["dirty"])
    cg.add_define("MIDEA_XYE_CONFIG_TARGET_TEMPERATURE_ENCODING", config[CONF_TARGET_TEMPERATURE_ENCODING])
    cg.add_define("MIDEA_XYE_CONFIG_SENSOR_TEMPERATURE_ENCODING", config[CONF_SENSOR_TEMPERATURE_ENCODING])
    cg.add_define("MIDEA_XYE_CONFIG_TARGET_TEMPERATURE_SOURCE", config[CONF_TARGET_TEMPERATURE_SOURCE])
    cg.add_define(
        "MIDEA_XYE_CONFIG_RAW_FAHRENHEIT_TARGET_TEMPERATURES",
        1 if raw_fahrenheit_target_temperatures else 0,
    )
    cg.add_define(
        "MIDEA_XYE_CONFIG_RAW_FAHRENHEIT_SENSOR_TEMPERATURES",
        1 if raw_fahrenheit_sensor_temperatures else 0,
    )
    cg.add_define("MIDEA_XYE_CONFIG_TARGET_TEMPERATURE_FROM_C0", 1 if target_temperature_from_c0 else 0)
    cg.add_define("MIDEA_XYE_CONFIG_USE_FAHRENHEIT", 1 if config[CONF_USE_FAHRENHEIT] else 0)
    cg.add_define(
        "MIDEA_XYE_CONFIG_SYNC_FAN_MODE_FROM_DEVICE",
        1 if config[CONF_SYNC_FAN_MODE_FROM_DEVICE] else 0,
    )
    return raw_fahrenheit_target_temperatures, raw_fahrenheit_sensor_temperatures, target_temperature_from_c0

validate_modes = cv.enum(ALLOWED_CLIMATE_MODES, upper=True)
validate_presets = cv.enum(ALLOWED_CLIMATE_PRESETS, upper=True)
validate_swing_modes = cv.enum(ALLOWED_CLIMATE_SWING_MODES, upper=True)
validate_custom_fan_modes = cv.enum(CUSTOM_FAN_MODES, upper=True)
validate_custom_presets = cv.enum(CUSTOM_PRESETS, upper=True)
validate_temperature_encoding = cv.enum(TEMPERATURE_ENCODINGS, upper=True)
validate_target_temperature_source = cv.enum(TARGET_TEMPERATURE_SOURCES, upper=True)

CONFIG_SCHEMA = cv.All(
    climate.climate_schema(ClimateMideaXYE).extend(
        {
            cv.GenerateID(): cv.declare_id(ClimateMideaXYE),
            cv.Optional(CONF_PERIOD, default="1s"): cv.time_period,
            cv.Optional(CONF_TIMEOUT, default="100ms"): cv.time_period,
            cv.Optional(CONF_USE_FAHRENHEIT, default=False): cv.boolean,
            cv.Optional(CONF_TARGET_TEMPERATURE_ENCODING, default="STANDARD"): validate_temperature_encoding,
            cv.Optional(CONF_SENSOR_TEMPERATURE_ENCODING, default="STANDARD"): validate_temperature_encoding,
            cv.Optional(CONF_TARGET_TEMPERATURE_SOURCE, default="C4"): validate_target_temperature_source,
            # Opt-in: derive the climate action from the C0 byte-19 compressor flag
            # and the defrost state. Off by default while byte 19 is still provisional;
            # legacy "fan running implies heating/cooling" behaviour is used when false.
            cv.Optional(CONF_COMPRESSOR_AWARE_ACTION, default=False): cv.boolean,
            # Opt-in: sync this->fan_mode from C4 target_fan_speed (commanded speed from the
            # physical thermostat), so Home Assistant reflects fan mode changes even when
            # not triggered by an HA command.
            cv.Optional(CONF_SYNC_FAN_MODE_FROM_DEVICE, default=False): cv.boolean,
            cv.OnlyWith(CONF_TRANSMITTER_ID, "remote_transmitter"): cv.use_id(
                remote_transmitter.RemoteTransmitterComponent
            ),
            cv.Optional(CONF_BEEPER, default=False): cv.boolean,
            cv.Optional(CONF_AUTOCONF, default=True): cv.boolean,
            cv.Optional(CONF_SUPPORTED_MODES): cv.ensure_list(validate_modes),
            cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(
                validate_swing_modes
            ),
            cv.Optional(CONF_SUPPORTED_PRESETS): cv.ensure_list(validate_presets),
            cv.Optional(CONF_CUSTOM_PRESETS): cv.ensure_list(validate_custom_presets),
            cv.Optional(CONF_CUSTOM_FAN_MODES): cv.ensure_list(
                validate_custom_fan_modes
            ),
            cv.Optional(CONF_STATIC_PRESSURE): number.number_schema(StaticPressureNumber).extend({
                cv.GenerateID(): cv.declare_id(StaticPressureNumber),
                cv.Optional(CONF_MIN_VALUE, default=0): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=15): cv.float_,
                cv.Optional(CONF_ICON, default="mdi:gauge"): cv.icon,
                cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
            }),
            cv.Optional(CONF_OUTDOOR_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Physical fan speed as a monotonic ordinal: 0=off, 1=low, 2=medium, 3=high.
            # The raw protocol nibble is not monotonic, so it is remapped before publishing.
            cv.Optional(CONF_FAN_SPEED): sensor.sensor_schema(
                icon="mdi:fan",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_TEMPERATURE_2A): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_2B): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_3): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                icon=ICON_POWER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TIMER_START): sensor.sensor_schema(
                unit_of_measurement=UNIT_MINUTE,
                icon=ICON_TIMER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DURATION,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TIMER_STOP): sensor.sensor_schema(
                unit_of_measurement=UNIT_MINUTE,
                icon=ICON_TIMER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DURATION,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ERROR_FLAGS): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_BUG,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_PROTECT_FLAGS): sensor.sensor_schema(
                unit_of_measurement=UNIT_EMPTY,
                icon=ICON_SECURITY,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_EMPTY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),

            cv.Optional(CONF_POWER_USAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                icon=ICON_POWER,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_HUMIDITY_SETPOINT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                icon=ICON_WATER_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FOLLOW_ME_SENSOR): cv.use_id(sensor.Sensor),
            cv.Optional(CONF_INTERNAL_CURRENT_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_INTERNAL_TARGET_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                icon=ICON_THERMOMETER,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DEFROST): binary_sensor.binary_sensor_schema(
                icon="mdi:snowflake-thermometer",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Optional(CONF_COMPRESSOR_ACTIVE): binary_sensor.binary_sensor_schema(
                icon="mdi:engine",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_with_arduino,
)

# Actions
FollowMeAction = midea_xye_ns.class_("FollowMeAction", automation.Action)
DisplayToggleAction = midea_xye_ns.class_("DisplayToggleAction", automation.Action)
SwingStepAction = midea_xye_ns.class_("SwingStepAction", automation.Action)
BeeperOnAction = midea_xye_ns.class_("BeeperOnAction", automation.Action)
BeeperOffAction = midea_xye_ns.class_("BeeperOffAction", automation.Action)
PowerOnAction = midea_xye_ns.class_("PowerOnAction", automation.Action)
PowerOffAction = midea_xye_ns.class_("PowerOffAction", automation.Action)
PowerToggleAction = midea_xye_ns.class_("PowerToggleAction", automation.Action)

MIDEA_ACTION_BASE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.use_id(ClimateMideaXYE),
    }
)

# FollowMe action
MIDEA_FOLLOW_ME_MIN = 0
MIDEA_FOLLOW_ME_MAX = 37
MIDEA_FOLLOW_ME_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TEMPERATURE): cv.templatable(cv.temperature),
        cv.Optional(CONF_BEEPER, default=False): cv.templatable(cv.boolean),
    }
)


@register_action("follow_me", FollowMeAction, MIDEA_FOLLOW_ME_SCHEMA)
async def follow_me_to_code(var, config, args):
    template_ = await cg.templatable(config[CONF_BEEPER], args, cg.bool_)
    cg.add(var.set_beeper(template_))
    template_ = await cg.templatable(config[CONF_TEMPERATURE], args, cg.float_)
    cg.add(var.set_temperature(template_))


# Toggle Display action
@register_action(
    "display_toggle",
    DisplayToggleAction,
    cv.Schema({}),
)
async def display_toggle_to_code(var, config, args):
    pass


# Swing Step action
@register_action(
    "swing_step",
    SwingStepAction,
    cv.Schema({}),
)
async def swing_step_to_code(var, config, args):
    pass


# Beeper On action
@register_action(
    "beeper_on",
    BeeperOnAction,
    cv.Schema({}),
)
async def beeper_on_to_code(var, config, args):
    pass


# Beeper Off action
@register_action(
    "beeper_off",
    BeeperOffAction,
    cv.Schema({}),
)
async def beeper_off_to_code(var, config, args):
    pass


# Power On action
@register_action(
    "power_on",
    PowerOnAction,
    cv.Schema({}),
)
async def power_on_to_code(var, config, args):
    pass


# Power Off action
@register_action(
    "power_off",
    PowerOffAction,
    cv.Schema({}),
)
async def power_off_to_code(var, config, args):
    pass


# Power Toggle action
@register_action(
    "power_toggle",
    PowerToggleAction,
    cv.Schema({}),
)
async def power_inv_to_code(var, config, args):
    pass


async def to_code(config):
    (
        raw_fahrenheit_target_temperatures,
        raw_fahrenheit_sensor_temperatures,
        target_temperature_from_c0,
    ) = _add_git_build_defines(config)

    var = await climate.new_climate(config)
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    await climate.register_climate(var, config)
    cg.add(var.set_period(config[CONF_PERIOD].total_milliseconds))
    cg.add(var.set_response_timeout(config[CONF_TIMEOUT].total_milliseconds))
    cg.add(var.set_use_fahrenheit(config[CONF_USE_FAHRENHEIT]))
    cg.add(var.set_raw_fahrenheit_target_temperatures(raw_fahrenheit_target_temperatures))
    cg.add(var.set_raw_fahrenheit_sensor_temperatures(raw_fahrenheit_sensor_temperatures))
    cg.add(var.set_target_temperature_from_c0(target_temperature_from_c0))
    cg.add(var.set_compressor_aware_action(config[CONF_COMPRESSOR_AWARE_ACTION]))
    cg.add(var.set_sync_fan_mode_from_device(config[CONF_SYNC_FAN_MODE_FROM_DEVICE]))
    if CONF_TRANSMITTER_ID in config:
        cg.add_define("USE_REMOTE_TRANSMITTER")
        transmitter_ = await cg.get_variable(config[CONF_TRANSMITTER_ID])
        cg.add(var.set_transmitter(transmitter_))
    if CONF_SUPPORTED_MODES in config:
        cg.add(var.set_supported_modes(config[CONF_SUPPORTED_MODES]))
    if CONF_SUPPORTED_SWING_MODES in config:
        cg.add(var.set_supported_swing_modes(config[CONF_SUPPORTED_SWING_MODES]))
    if CONF_SUPPORTED_PRESETS in config:
        cg.add(var.set_supported_presets(config[CONF_SUPPORTED_PRESETS]))
    if CONF_CUSTOM_PRESETS in config:
        cg.add(var.set_custom_presets(config[CONF_CUSTOM_PRESETS]))
    if CONF_CUSTOM_FAN_MODES in config:
        cg.add(var.set_custom_fan_modes(config[CONF_CUSTOM_FAN_MODES]))
    if CONF_STATIC_PRESSURE in config:
        static_pressure_var = await number.new_number(
            config[CONF_STATIC_PRESSURE],
            min_value=config[CONF_STATIC_PRESSURE][CONF_MIN_VALUE],
            max_value=config[CONF_STATIC_PRESSURE][CONF_MAX_VALUE],
            step=1
        )
        cg.add(var.set_static_pressure_number(static_pressure_var))
    if CONF_OUTDOOR_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_OUTDOOR_TEMPERATURE])
        cg.add(var.set_outdoor_temperature_sensor(sens))
    if CONF_FAN_SPEED in config:
        sens = await sensor.new_sensor(config[CONF_FAN_SPEED])
        cg.add(var.set_fan_speed_sensor(sens))
    if CONF_TEMPERATURE_2A in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_2A])
        cg.add(var.set_temperature_2a_sensor(sens))
    if CONF_TEMPERATURE_2B in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_2B])
        cg.add(var.set_temperature_2b_sensor(sens))
    if CONF_TEMPERATURE_3 in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_3])
        cg.add(var.set_temperature_3_sensor(sens))
    if CONF_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))
    if CONF_TIMER_START in config:
        sens = await sensor.new_sensor(config[CONF_TIMER_START])
        cg.add(var.set_timer_start_sensor(sens))
    if CONF_TIMER_STOP in config:
        sens = await sensor.new_sensor(config[CONF_TIMER_STOP])
        cg.add(var.set_timer_stop_sensor(sens))
    if CONF_ERROR_FLAGS in config:
        sens = await sensor.new_sensor(config[CONF_ERROR_FLAGS])
        cg.add(var.set_error_flags_sensor(sens))
    if CONF_PROTECT_FLAGS in config:
        sens = await sensor.new_sensor(config[CONF_PROTECT_FLAGS])
        cg.add(var.set_protect_flags_sensor(sens))
    if CONF_POWER_USAGE in config:
        sens = await sensor.new_sensor(config[CONF_POWER_USAGE])
        cg.add(var.set_power_sensor(sens))
    if CONF_HUMIDITY_SETPOINT in config:
        sens = await sensor.new_sensor(config[CONF_HUMIDITY_SETPOINT])
        cg.add(var.set_humidity_setpoint_sensor(sens))
    if CONF_FOLLOW_ME_SENSOR in config:
        sens = await cg.get_variable(config[CONF_FOLLOW_ME_SENSOR])
        cg.add(var.set_follow_me_sensor(sens))
    if CONF_INTERNAL_CURRENT_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INTERNAL_CURRENT_TEMPERATURE])
        cg.add(var.set_internal_current_temperature_sensor(sens))
    if CONF_INTERNAL_TARGET_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_INTERNAL_TARGET_TEMPERATURE])
        cg.add(var.set_internal_target_temperature_sensor(sens))
    if CONF_DEFROST in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_DEFROST])
        cg.add(var.set_defrost_sensor(sens))
    if CONF_COMPRESSOR_ACTIVE in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_COMPRESSOR_ACTIVE])
        cg.add(var.set_compressor_active_sensor(sens))
