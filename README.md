# ESPHome-Midea-XYE

ESPHome external component for controlling Midea HVAC systems over the XYE/CCM RS-485 bus. Provides a native Home Assistant climate entity with full mode, fan, and setpoint support.

## Overview

This component communicates with Midea-like air conditioners (heat pumps) via the XYE protocol over RS-485.

For detailed protocol documentation, see [PROTOCOL.md](esphome/components/midea_xye/PROTOCOL.md).

### Acknowledgments

Kudos to these projects and people:
- Reverse engineering of the protocol: https://codeberg.org/xye/xye
- Working implementation using ESP32 by @Bunicutz: https://github.com/Bunicutz/ESP32_Midea_RS485
- Working implementation by @wtahler: https://github.com/wtahler/esphome-mideaXYE-rs485
- Fully integrated Midea Climate component: https://github.com/esphome/esphome/tree/dev/esphome/components/midea
- ESPHome external component foundation by @exciton: https://github.com/exciton/esphome
- Key contributions and inspiration by @mdrobnak: https://github.com/mdrobnak/esphome/tree/units_switch
- Static pressure protocol analysis by @rymo
- Home Assistant community discussion and contributions: https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679

## Hardware Requirements

- ESP8266 or ESP32 board (e.g., D1 Mini)
- RS-485 to TTL converter dongle
- Connection to your Midea HVAC unit's XYE/CCM RS-485 bus

## Installation

Add this external component to your ESPHome configuration.

**Recommended: pin to a specific release tag for stability:**

```yaml
external_components:
  - source: 
      type: git
      url: https://github.com/HomeOps/ESPHome-Midea-XYE
      ref: vX.Y.Z  # replace with the latest release tag
    components: [midea_xye]
```

Or use the latest development version from the `main` branch:

```yaml
external_components:
  - source: 
      type: git
      url: https://github.com/HomeOps/ESPHome-Midea-XYE
      ref: main
    components: [midea_xye]
```

See the [Releases page](https://github.com/HomeOps/ESPHome-Midea-XYE/releases) for available versions and changelogs.

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
  # Optional: Enable debug logging for XYE protocol messages
  # logs:
  #   midea_xye: DEBUG

external_components:
  - source: 
      type: git
      url: https://github.com/HomeOps/ESPHome-Midea-XYE
      ref: v1.0.0  # replace with the latest release tag
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

## Debugging

### Enabling Protocol Debug Logging

To see detailed XYE protocol messages (useful for troubleshooting or protocol reverse engineering), enable debug logging for the component:

```yaml
logger:
  baud_rate: 0  # Required: Disable UART logging to avoid conflicts with RS-485
  logs:
    midea_xye: DEBUG  # Enable debug-level logging for XYE protocol
```

With debug logging enabled, you'll see:
- **TX Messages**: Outgoing messages sent to the HVAC unit (operation mode, fan mode, target temperature, etc.)
- **RX Messages**: Incoming messages from the HVAC unit (current state, temperatures, flags, etc.)
- All field values are shown in hex format (e.g., `0x50 (20.0°C)`)
- Enum values show both name and hex (e.g., `0x88 (COOL)` or `0x99 (UNKNOWN)`)
- Bounds checking prevents displaying garbage data from truncated messages

Example debug output:
```
[D][midea_xye:186] TX Message:
[D][midea_xye:186]   Frame Header:
[D][midea_xye:186]     command: 0xC0 (QUERY)
[D][midea_xye:186]   TransmitMessageData:
[D][midea_xye:186]     operation_mode: 0x88 (COOL)
[D][midea_xye:186]     fan_mode: 0x80 (FAN_AUTO)
[D][midea_xye:186]     target_temperature: 0x50 (20.0°C)
```

## Features

### What Works
- Setting mode (off, fan, cool, heat, dry, auto/HEAT_COOL; see note below — this does not make the indoor unit switch modes on its own)
- Setting temperature (can send in Celsius or Fahrenheit; handles AC results in both; must manually set in YAML)
- Setting fan mode (auto, low, medium, high)
- Reading inside and outside air temperatures
- Reading inside coil temperature and outside coil temperature
- Reading timer start/stop times (set by remote)
- Follow-Me temperature - automatically sends room temperature from a configured sensor to the AC unit. Updates on sensor state changes and every 30 seconds.
- Reading current mode (HEAT or COOL) as commanded by the Midea proprietary thermostat when one is connected to the XYE bus

> **Note on AUTO mode:** The **indoor unit itself has no AUTO mode** — it can only ever be
> in HEAT or COOL. The apparent "AUTO" behaviour comes entirely from the **Midea proprietary
> thermostat (tstat)**, which contains its own internal temperature sensor and runs the
> switching logic on-device: it periodically sends HEAT or COOL commands to the indoor unit
> based on that sensor reading.  When the Midea tstat is disconnected from the bus the indoor
> unit stays in whichever mode it was last commanded — it will **never** switch on its own.
> This component can read the current HEAT/COOL state that the tstat commanded, but has no
> access to the tstat's internal sensor and cannot replicate its logic.  See the
> [Smart Thermostat](#smart-thermostat) section below for a Home Assistant–level replacement.

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

## Smart Thermostat

### Why AUTO mode requires a smart thermostat component

The **Midea indoor unit has no AUTO mode of its own** — it can only operate in HEAT or COOL
(or FAN, DRY, OFF). The switching logic that makes it feel like "auto" lives entirely inside
the **Midea proprietary thermostat (tstat)**, a separate device connected to the same XYE/CCM
RS-485 bus. The tstat carries its own internal temperature sensor and runs a simple control
loop on-device: when the room is too cold it sends a HEAT command to the indoor unit; when the
room is too warm it sends a COOL command. The indoor unit just obeys.

**If you remove the Midea tstat from the bus, the indoor unit will never switch between HEAT
and COOL on its own** — it stays in whichever mode it received last.

This ESP component cannot replicate the tstat's role because:

1. The tstat's internal sensor is proprietary and inaccessible over the XYE bus.
2. The component does not currently implement the built-in smart thermostat control loop that
   periodically decides when to send the HEAT/COOL switching commands the tstat normally provides.

A software replacement that replicates exactly what the Midea tstat does — but using
Home Assistant sensors instead of the proprietary hardware sensor, and adding configurable
comfort bands, named presets (Home / Sleep / Away), and outside-temperature awareness — is the
companion HACS component:

👉 **[HomeOps/HASS-Smart-Climate](https://github.com/HomeOps/HASS-Smart-Climate)**

Install it via HACS, point it at the climate entity created by this component (for example,
`climate.<your_name>`), and configure your comfort temperature ranges for each preset. The
smart thermostat then drives the real device, sending HEAT or COOL commands exactly as the
Midea tstat would, while keeping its setpoint at the midpoint of the active range.

## Community

For additional guidance and community support, visit the Home Assistant Community discussion:
https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679

## License

See [LICENSE](LICENSE) file for details.
