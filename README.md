# ESPHome-Midea-XYE

ESPHome external component for controlling Midea HVAC systems over the XYE/CCM RS-485 bus. Provides a native Home Assistant climate entity with full mode, fan, and setpoint support.

## Overview

This component communicates with Midea-like air conditioners (heat pumps) via the XYE protocol over RS-485.

### Acknowledgments

Kudos to these projects:
- Reverse engineering of the protocol: https://codeberg.org/xye/xye
- Working implementation using ESP32: https://github.com/Bunicutz/ESP32_Midea_RS485
- Fully integrated Midea Climate component: https://github.com/esphome/esphome/tree/dev/esphome/components/midea

## Hardware Requirements

- ESP8266 or ESP32 board (e.g., D1 Mini)
- RS-485 to TTL converter dongle
- Connection to your Midea HVAC unit's XYE/CCM RS-485 bus

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

# UART settings for RS-485 converter dongle (required)
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

### Follow-Me Example

The Follow-Me feature allows the AC unit to use a remote temperature sensor (like one in your living room) instead of its built-in sensor. This provides better temperature control for the entire room.

```yaml
# Temperature sensor (example using Home Assistant)
sensor:
  - platform: homeassistant
    id: living_room_temp
    entity_id: sensor.living_room_temperature

climate:
  - platform: midea_xye
    name: Heatpump
    follow_me_sensor: living_room_temp  # AC uses this sensor for temperature readings
```

The component will automatically:
- Send temperature updates when the sensor value changes
- Send periodic updates every 30 seconds to keep the AC informed
- No lambda or automation needed!


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
    # NOTE: Experimental / not yet fully implemented:
    #   - SILENT and TURBO fan modes are defined but currently not used in the
    #     device control logic and may have no effect.
    custom_fan_modes:           # Optional
      - SILENT
      - TURBO
    # NOTE: Experimental / not yet fully implemented:
    #   - Presets such as BOOST and SLEEP are defined but currently not used in
    #     the device control logic and may have no effect.
    supported_presets:          # Optional
      - BOOST
      - SLEEP
    supported_swing_modes:      # Optional
      - VERTICAL
    follow_me_sensor: room_temp_sensor  # Optional. Automatically sends room temperature to AC for better temperature control
                                        # The sensor is updated on state change and every 30 seconds
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
- Follow-Me temperature - automatically sends room temperature from a configured sensor to the AC unit. Updates on sensor state changes and every 30 seconds.

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
