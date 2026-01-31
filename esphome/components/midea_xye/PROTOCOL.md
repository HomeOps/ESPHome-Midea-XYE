# XYE/CCM Protocol Documentation

## Overview

This document describes the Midea XYE/CCM RS-485 protocol used for communication between thermostats and HVAC units. The protocol uses a master-slave architecture where the thermostat (client) initiates communication and the HVAC unit (server) responds.

## Acknowledgments

This documentation is based on:
- Reverse engineering work at: https://codeberg.org/xye/xye
- Implementation by wtahler: https://github.com/wtahler/esphome-mideaXYE-rs485
- Implementation by Bunicutz: https://github.com/Bunicutz/ESP32_Midea_RS485
- ESPHome Midea component: https://github.com/esphome/esphome/tree/dev/esphome/components/midea

## Physical Layer

- **Interface**: RS-485 differential signaling (X is A, Y is B, E is GND)
- **Baud Rate**: 4800 bps
- **Data Format**: 8 bits, no parity, 1 stop bit (8N1)
- **Bus**: Half-duplex
- **Architecture**: Master/slave - CCM (master) polls all possible 64 unit IDs
- **Timing**: 130ms time slice per unit (30ms query + 100ms timeout for response)

## Message Structure

All messages start with a preamble byte (0xAA) and end with a CRC byte followed by a prologue byte (0x55).

### Node Addressing

- **Device IDs**: 0x00 to 0x3F (64 possible unit addresses)
- **Broadcast ID**: 0xFF (messages sent to all units)
- **Master ID**: Typically 0x00 (CCM/thermostat)

### Transmit Messages (Client → Server)

**Length**: 16 bytes

```
Byte    Field               Description
----    -----               -----------
0       Preamble            Always 0xAA
1       Command             Command type (see Commands section)
2       Destination ID      HVAC unit ID (0x00..0x3F for specific unit, 0xFF for broadcast)
3       Source ID           Thermostat/master ID (0x00..0x3F)
4       Direction           Direction flag (0x80 when from master)
5       Source ID (repeat)  Thermostat/master ID (0x00..0x3F)
6       Operation Mode      Mode byte (see Operation Modes)
7       Fan Mode            Fan speed byte (see Fan Modes)
8       Temperature         Target temperature (see Temperature Encoding)
9       Timer Start         Timer start flags (combinable, see Timer Flags)
10      Timer Stop          Timer stop flags (combinable, see Timer Flags)
11      Mode Flags          Special mode flags (see Mode Flags)
12      Reserved            Reserved/unused (0x00)
13      Complement          Bitwise complement of command (0xFF - Command)
14      CRC                 Checksum (see CRC Calculation)
15      Prologue            Always 0x55
```

### Receive Messages (Server → Client)

**Length**: 32 bytes

```
Byte    Field               Description
----    -----               -----------
0       Preamble            Always 0xAA
1       Command             Response command type (echoes request command)
2       Direction           Direction flag (0x00 in practice, though some docs specify 0x80)
3       Destination ID      Master/thermostat ID (0x00..0x3F)
4       Source ID           Unit/device ID (0x00..0x3F)
5       Destination ID      Master/thermostat ID (repeated)
6       Unknown1            Unknown/reserved (possibly 0x30 - capabilities related)
7       Capabilities        Unit capability flags
8       Operation Mode      Current operation mode
9       Fan Mode            Current fan speed
10      Target Temp         Target temperature setpoint
11      T1 Temperature      Internal/room temperature sensor
12      T2A Temperature     Indoor coil inlet temperature
13      T2B Temperature     Indoor coil outlet temperature
14      T3 Temperature      Outdoor coil/ambient temperature
15      Current             Current draw (units unknown, often reads 0xFF)
16      Unknown2            Unknown/reserved
17      Timer Start         Start timer setting
18      Timer Stop          Stop timer setting
19      Unknown3            Unknown/reserved
20      Mode Flags          Special mode flags
21      Operation Flags     Status flags (water pump, water lock, etc.)
22      Error Flags Low     Error code/flags (low byte)
23      Error Flags High    Error code/flags (high byte)
24      Protect Flags Low   Protection flags (low byte)
25      Protect Flags High  Protection flags (high byte)
26      CCM Error Flags     Communication error flags
27      Unknown4            Unknown/reserved
28      Unknown5            Unknown/reserved
29      Unknown6            Unknown/reserved
30      CRC                 Checksum
31      Prologue            Always 0x55
```

## Commands

Commands are sent from the client (thermostat) to the server (HVAC unit):

```
Command             Value   Description
-------             -----   -----------
QUERY               0xC0    Query current status (basic)
QUERY_EXTENDED      0xC4    Query extended status (outdoor temp, static pressure)
SET                 0xC3    Set operation parameters (mode, fan, temperature)
FOLLOW_ME           0xC6    Send remote temperature reading for Follow-Me feature
LOCK                0xCC    Lock physical controls
UNLOCK              0xCD    Unlock physical controls
```

The server responds with the same command code.

## Operation Modes

```
Mode        Value   Description                 Notes
----        -----   -----------                 -----
OFF         0x00    Unit is off                 
AUTO        0x80    Automatic mode              
AUTO_ALT    0x91    Automatic mode (alternate)  Observed in some implementations (0x80 | 0x10 | 0x01)
FAN         0x81    Fan only mode               Derived from AUTO (0x80 | 0x01)
DRY         0x82    Dehumidify mode             Derived from AUTO (0x80 | 0x02)
HEAT        0x84    Heating mode                Derived from AUTO (0x80 | 0x04)
COOL        0x88    Cooling mode                Derived from AUTO (0x80 | 0x08)
```

**Note**: Some implementations use AUTO_ALT (0x91 = 0x80 | 0x10 | 0x01), which includes the OP_MODE_AUTO_FLAG (0x10). The exact encoding may vary by unit model.

## Fan Modes

```
Mode        Value   Description                 Notes
----        -----   -----------                 -----
FAN_OFF     0x00    Fan off                     
FAN_HIGH    0x01    High speed                  
FAN_MEDIUM  0x02    Medium speed                
FAN_LOW_ALT 0x03    Low speed (alternate)       Observed in some implementations
FAN_LOW     0x04    Low speed                   
FAN_AUTO    0x80    Automatic fan speed         
```

**Note**: Low fan speed has been observed as both FAN_LOW_ALT (0x03) and FAN_LOW (0x04) in different implementations. Use the value that works with your specific unit.

## Temperature Encoding

### Celsius Encoding (Default)
```
encoded_value = (celsius * 2.0) + 0x28
celsius = (encoded_value - 0x28) / 2.0
```

**Alternative formulation** (from codeberg.org/xye/xye):
```
encoded_value = (celsius / 0.5) + 0x30
```
Both formulations are equivalent (0x28 = 40 decimal, 0x30 = 48 decimal in different contexts).

**Example**: 
- 20°C → (20 * 2) + 0x28 = 0x50 (80 decimal)
- 0x50 → (80 - 40) / 2.0 = 20°C

### Fahrenheit Encoding
Some implementations send raw Fahrenheit values directly without encoding. The behavior may depend on unit configuration or regional settings.

**Special Value**:
- `0xFF`: Used in FAN mode when temperature control is not applicable

## Mode Flags

Special operation modes (byte 11 in transmit, byte 20 in receive):

```
Flag            Value   Description
----            -----   -----------
NORMAL          0x00    Normal operation
ECO             0x01    Energy saving mode
AUX_HEAT        0x02    Auxiliary heat / Turbo mode
SWING           0x04    Swing/oscillation mode
VENTILATION     0x88    Ventilation mode
```

## Timer Flags

Timer values are encoded as combinable bit flags (bytes 9-10 in transmit, bytes 17-18 in receive):

```
Flag            Value   Duration
----            -----   --------
TIMER_15MIN     0x01    15 minutes
TIMER_30MIN     0x02    30 minutes
TIMER_1HOUR     0x04    1 hour
TIMER_2HOUR     0x08    2 hours
TIMER_4HOUR     0x10    4 hours
TIMER_8HOUR     0x20    8 hours
TIMER_16HOUR    0x40    16 hours
INVALID         0x80    Timer not set/invalid
```

Flags can be combined to create specific durations. For example:
- 1.5 hours = TIMER_1HOUR | TIMER_30MIN = 0x06
- 3 hours = TIMER_2HOUR | TIMER_1HOUR = 0x0C

## Operation Flags

Status flags from the unit (byte 21 in receive):

```
Flag            Value   Description
----            -----   -----------
WATER_PUMP      0x04    Water pump is active
WATER_LOCK      0x80    Water lock protection is active
```

## Capabilities Flags

Unit capability flags (byte 7 in receive):

```
Flag            Value   Description
----            -----   -----------
EXTERNAL_TEMP   0x80    Supports external temperature sensor (Follow-Me)
SWING           0x10    Supports swing/oscillation function
```

## Error and Protection Flags

Error flags are reported as a 16-bit value across bytes 22-23:
- Byte 22: Error flags low byte (E1 error codes)
- Byte 23: Error flags high byte (E2 error codes)

Protection flags are reported as a 16-bit value across bytes 24-25:
- Byte 24: Protection flags low byte
- Byte 25: Protection flags high byte

The exact meaning of individual error codes varies by unit model and requires the service manual.

## CCM Communication Error Flags

Communication error flags (byte 26 in receive):

```
Flag            Value   Description
----            -----   -----------
NO_ERROR        0x00    No communication errors
TIMEOUT         0x01    Communication timeout
CRC_ERROR       0x02    CRC check failed
PROTOCOL_ERROR  0x04    Protocol violation
```

## CRC Calculation

The CRC is a simple checksum calculated as follows:

```
1. Sum all bytes in the message EXCEPT the CRC byte and prologue
2. Take the least significant byte of the sum (sum & 0xFF)
3. CRC = 0xFF - (sum & 0xFF)
```

**Example** (16-byte transmit message):
```c
uint8_t crc = 0;
for (int i = 0; i < 14; i++) {  // Bytes 0-13 (exclude CRC at 14 and prologue at 15)
    crc += message[i];
}
message[14] = 0xFF - crc;  // Store CRC
message[15] = 0x55;        // Store prologue
```

## Follow-Me Subcommands

When using the FOLLOW_ME (0xC6) command, byte 10 contains a subcommand:

```
Subcommand          Value   Description
----------          -----   -----------
UPDATE              0x02    Regular temperature update
STATIC_PRESSURE     0x04    Static pressure setting
INIT                0x06    Initialization after mode change
```

## Communication Flow

### Polling Architecture
The master (CCM/thermostat) uses a polling model:
1. Master polls all 64 possible unit IDs (0x00 to 0x3F) sequentially
2. Each unit gets a 130ms time slice:
   - 30ms for sending query/command
   - 100ms timeout waiting for response
3. Units only respond when addressed by their specific ID
4. Broadcast messages (0xFF) are received by all units but typically don't generate responses

### Basic Query Cycle
1. Client sends QUERY (0xC0) command to specific unit ID
2. Server responds with current status (if ID matches)
3. Client processes response
4. Continue polling next unit ID or repeat after delay

### Setting Parameters
1. Client sends SET (0xC3) command with new parameters
2. Server acknowledges and applies settings
3. Server responds with updated status
4. Client may send QUERY to confirm changes

### Broadcast Commands
1. Client sends command with destination ID = 0xFF
2. All units receive and process the command
3. No response expected from units
4. Useful for simultaneous updates to all units

### Follow-Me Operation
1. Client sends FOLLOW_ME (0xC6) with room temperature
2. Server uses client temperature instead of internal sensor
3. Client sends updates when temperature changes
4. Client sends periodic updates (every 30 seconds) to maintain Follow-Me mode

## Implementation Notes

### Timing Considerations
- Master polls with 130ms time slice per unit (30ms send + 100ms timeout)
- Allow 100-200ms for HVAC unit to respond to commands
- Wait for response before sending next command to avoid bus collisions
- Query status periodically (10-15 seconds) to maintain communication
- Send Follow-Me updates every 30 seconds minimum

### Error Handling
- Validate preamble (0xAA) and prologue (0x55) on received messages
- Verify CRC before processing message data
- Implement timeout for responses (typically 500ms-1s)
- Retry failed commands with exponential backoff

### Multi-Unit Systems
- The bus supports up to 64 units with IDs 0x00 to 0x3F
- Each unit must have a unique node ID
- Master polls all possible IDs sequentially (even if no unit is present)
- Address specific units by setting the destination ID (byte 2) in transmit messages
- Use broadcast ID (0xFF) to send commands to all units simultaneously
- Monitor bus for responses to identify which units are present on the system

## Known Issues and Variations

### Direction Flag (Byte 2)
- Some protocol documentation specifies 0x80 for responses from AC to thermostat
- In practice, AC units send 0x00 in byte 2 for all responses
- The implementation uses 0x00 to match actual AC behavior
- Both request and response messages use 0x00 in the direction byte

### Temperature Encoding
- Some units use raw Fahrenheit values without encoding
- Some units use Celsius encoding regardless of display setting
- The encoding formula may need adjustment for specific models

### Mode Values
- AUTO mode has been observed as both 0x80 and 0x91
- LOW fan has been observed as both 0x03 and 0x04
- Always test with your specific unit and adjust as needed

### Current Reading
- Current reading (byte 15) often returns 0xFF (invalid)
- Actual current measurement may not be supported on all models

### Extended Query
- QUERY_EXTENDED (0xC4) provides outdoor temperature and static pressure
- Not all fields in extended query response are fully understood
- Field interpretation may vary by model

## References

1. **XYE Reverse Engineering Project**
   - https://codeberg.org/xye/xye
   - Original reverse engineering work on the protocol

2. **wtahler's Implementation**
   - https://github.com/wtahler/esphome-mideaXYE-rs485
   - Working ESP implementation with field mappings

3. **Bunicutz's ESP32 Implementation**
   - https://github.com/Bunicutz/ESP32_Midea_RS485
   - Another working implementation with insights

4. **ESPHome Midea Component**
   - https://github.com/esphome/esphome/tree/dev/esphome/components/midea
   - Related Midea protocol (different from XYE)

## Version History

- **v1.0** (2026-01-30): Initial documentation based on code analysis and external references
