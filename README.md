# ESPHome-Midea-XYE

ESPHome external component for controlling Midea HVAC systems over the XYE/CCM RS‑485 bus. Provides a native Home Assistant climate entity with full mode, fan, and setpoint support.

## Overview

This component communicates with Midea-like air conditioners (heat pumps) via the XYE protocol over RS485.

### Acknowledgments

Kudos to these projects:
- Reverse engineering of the protocol: https://codeberg.org/xye/xye
- Working implementation using ESP32: https://github.com/Bunicutz/ESP32_Midea_RS485
- Fully integrated Midea Climate component: https://github.com/esphome/esphome/tree/dev/esphome/components/midea

## Hardware Requirements

- ESP8266 or ESP32 board (e.g., D1 Mini)
- RS485 to TTL converter dongle
- Connection to your Midea HVAC unit's XYE/CCM RS485 bus

## Installation

Add this external component to your ESPHome configuration:

```yaml
external_components:
  - source: 
      type: git
      url: https://github.com/HomeOps/ESPHome-Midea-XYE
      ref: main
    components: [midea_xye]
```

## Configuration

### Basic Example

```yaml
esphome:
  name: heatpump
  friendly_name: Heatpump

esp8266:  # also works with esp32
  board: d1_mini

# Enable logging (but not via UART)
logger:
  baud_rate: 0

external_components:
  - source: 
      type: git
      url: https://github.com/HomeOps/ESPHome-Midea-XYE
      ref: main
    components: [midea_xye]

# UART settings for RS485 converter dongle (required)
uart:
  tx_pin: TX
  rx_pin: RX
  baud_rate: 4800
  debug:  # If you want to help reverse engineer
    direction: BOTH

# Main settings
climate:
  - platform: midea_xye
    name: Heatpump
    period: 1s                  # Optional. Defaults to 1s
    timeout: 100ms              # Optional. Defaults to 100ms
    use_fahrenheit: false       # Optional. Defaults to false
```

### Advanced Configuration

```yaml
climate:
  - platform: midea_xye
    name: Heatpump
    period: 1s                  # Optional. Defaults to 1s
    timeout: 100ms              # Optional. Defaults to 100ms
    use_fahrenheit: false       # Optional. Defaults to false
    #beeper: true               # Optional. Beep on commands
    visual:                     # Optional. Example of visual settings override
      min_temperature: 17 °C    # min: 17
      max_temperature: 30 °C    # max: 30
      temperature_step: 1.0 °C  # min: 0.5
    supported_modes:            # Optional
      - FAN_ONLY
      - HEAT_COOL              
      - COOL
      - HEAT
      - DRY
    custom_fan_modes:           # Optional
      - SILENT
      - TURBO
    supported_presets:          # Optional
      - BOOST
      - SLEEP
    supported_swing_modes:      # Optional
      - VERTICAL
    outdoor_temperature:        # Optional. Outdoor temperature sensor
      name: Outside Temp
    temperature_2a:             # Optional. Inside coil temperature
      name: Inside Coil Inlet Temp
    temperature_2b:             # Optional. Inside coil temperature
      name: Inside Coil Outlet Temp
    temperature_3:              # Optional. Outside coil temperature
      name: Outside Coil Temp
    current:                    # Optional. Current measurement
      name: Current
    timer_start:                # Optional. On timer duration
      name: Timer Start
    timer_stop:                 # Optional. Off timer duration
      name: Timer Stop
    error_flags:                # Optional
      name: Error Flags
    protect_flags:              # Optional
      name: Protect Flags
```

## Features

### What Works
- Setting mode (off, auto, fan, cool, heat, dry)
- Setting temperature (can send in Celsius or Fahrenheit; handles AC results in both; must manually set in YAML)
- Setting fan mode (auto, low, medium, high)
- Reading inside and outside air temperatures
- Reading inside coil temperature and outside coil temperature
- Reading timer start/stop times (set by remote)
- Follow-Me temperature (point it at a sensor and this works well)

### Known Issues
- Current reading always shows 255
- Setting swing mode not working

### Not Yet Implemented
- Setting timers directly to unit (not a high priority since automations can handle this)
- Forcing display to show Celsius or Fahrenheit (setting temp in Celsius doesn't force display to Celsius)
- Freeze protection
- Silent mode
- Lock/Unlock

### Not Tested
- IR integration (however, not needed for Follow-Me functionality)

## Community

For additional guidance and community support, visit the Home Assistant Community discussion:
https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679

## License

See [LICENSE](LICENSE) file for details.
