AUTO_LOAD = ["climate"]
DEPENDENCIES = ["climate", "uart"]

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate, uart
from esphome.const import CONF_ID

# Register this file as a climate *platform* named "cn40"
#climate.register_climate_platform("cn40")

# Create namespace for C++ class
cn40_ns = cg.esphome_ns.namespace("cn40")

# Bind to your C++ class
CN40Climate = cn40_ns.class_(
    "CN40Climate",
    climate.Climate,     # base class
    cg.Component,
    uart.UARTDevice,
)

# Platform schema
CONFIG_SCHEMA = climate._CLIMATE_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(CN40Climate),
}).extend(uart.UART_DEVICE_SCHEMA)

# Code generation
async def to_code(config):
    uart_component = await cg.get_variable(config[uart.CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_component)

    await cg.register_component(var, config)
    await climate.register_climate(var, config)
    await uart.register_uart_device(var, config)

