# XYE/CCM Protocol Documentation

## Overview

This document describes the Midea XYE/CCM RS-485 protocol used for communication between thermostats and HVAC units. The protocol uses a master-slave architecture where the thermostat (client) initiates communication and the HVAC unit (server) responds.

## Acknowledgments

This documentation is based on:
- Reverse engineering work at: https://codeberg.org/xye/xye
- Implementation by wtahler: https://github.com/wtahler/esphome-mideaXYE-rs485
- Implementation by Bunicutz: https://github.com/Bunicutz/ESP32_Midea_RS485
- ESPHome Midea component: https://github.com/esphome/esphome/tree/dev/esphome/components/midea
- HA Community thread reverse-engineering by mdrobnak, rymo, and others: https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679 ([archived PDF](https://github.com/user-attachments/files/26555742/Midea.A_C.via.local.XYE.-.ESPHome.-.Home.Assistant.Community.pdf))
- S1/S2 bus reverse-engineering by MidATRIX: https://github.com/MidATRIX/midea-s1s2-rs485-monitor — different bus (IDU↔ODU), same RS-485 medium, useful field-level cross-reference. See [Related Protocols](#related-protocols--s1s2-bus-iduodu).

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
2       Direction           Direction flag (0x00 in practice, though some docs specify 0x80).
                            Messages are distinguished by context and command type, not this byte.
3       Destination ID      Master/thermostat ID (0x00..0x3F)
4       Source ID           Unit/device ID (0x00..0x3F)
5       Destination ID      Master/thermostat ID (repeated)
6       Unknown1            Unknown/reserved (possibly 0x30 - capabilities related)
7       Capabilities        Unit capability flags
8       Operation Mode      Current operation mode
9       Fan Mode            Current fan speed
10      Target Temp         Target temperature setpoint (raw Celsius). Bit 6 (0x40) is a
                            status flag unrelated to the temperature — it must be masked out
                            with SET_TEMP_VALUE_MASK (0xBF) before interpreting the value.
11      T1 Temperature      Internal/room temperature sensor
12      T2A Temperature     Indoor coil inlet temperature
13      T2B Temperature     Indoor coil outlet temperature
14      T3 Temperature      Outdoor coil/ambient temperature
15      Current             Current draw (units unknown, often reads 0xFF)
16      Unknown2            Unknown/reserved
17      Timer Start         Start timer setting
18      Timer Stop          Stop timer setting
19      Compressor Run Flag Provisional compressor-running flag. `0x01` observed while
                            running and `0x00` while idling in heat-mode captures.
20      Mode Flags          Special mode flags
21      Operation Flags     Status flags (water pump, water lock, etc.)
22      Error Flags Low     Error code/flags (low byte)
23      Error Flags High    Error code/flags (high byte)
24      Protect Flags Low   Protection flags (low byte)
25      Protect Flags High  Protection flags (high byte)
26      CCM Error Flags     Communication error flags
27      Unknown4            Unknown. Hardware-dependent (0x00 or 0x14 observed),
                            steady within a given device.
28      Unknown5            Unknown. Observed varying over time on some hardware
                            (see "Byte 27-29 observations" below).
29      Unknown6            Unknown. Observed varying over time on some hardware
                            (see "Byte 27-29 observations" below).
30      CRC                 Checksum
31      Prologue            Always 0x55
```

### Byte 19 observation

- **Byte 19 (`Compressor Run Flag`)** — provisional compressor-running flag.
  In paired heat-mode captures from one system, C0 byte 19 was `0x01` while the
  compressor was running and `0x00` while the unit was idling, while the C4
  `compressor_flags` field stayed at `0x8C` (compressor active, outdoor fan
  running, no protections) and C0 bytes 28-29 (`0xE0/0x01`) stayed
  unchanged. This needs validation on more hardware before being treated as
  universal.

### C4 byte 17 (`Target Fan Speed`)

C4 (QUERY_EXTENDED) byte 17 — previously labelled `indoor_unit_address` — is the
target (commanded) fan speed. It uses the same encoding as the C0 `fan_mode` byte
(see [Fan Modes](#fan-modes)): `HIGH 0x01`, `MEDIUM 0x02`, `LOW 0x04`, `AUTO 0x80`.
Note that, as with C0 `fan_mode`, low fan has been observed as both `0x03` and `0x04`
on different units — do not assume `0x04` is universal. All four speeds were confirmed
by selecting each in turn and observing byte 17 directly (issue #120).

This is distinct from the C0 `fan_mode` byte, which reports the *actual* speed the fan
is currently running and reads `0x00` while the fan is idle — so byte 17 is the field
to use when reflecting the user's fan selection.

### Byte 27-29 observations

These three bytes are not yet decoded. Cross-referencing independent captures
yields the following partial picture:

- **Byte 27 (`Unknown4`)** — hardware-dependent, steady within a given device.
  `0x00` across 771 C0 responses on a ducted heat pump in the US Pacific
  Northwest; `0x14` across dozens of C0 responses on a C&H CH-36AHU
  (Midea-manufactured; Home Assistant community thread
  ["Midea A/C via Local XYE"](https://community.home-assistant.io/t/midea-a-c-via-local-xye/),
  mdrobnak). Likely a capability / model-class byte.
- **Byte 28 (`Unknown5`)** — *not* a static status word and *not* a monotonic
  counter. A ~22-minute idle capture from a PNW ducted heat pump recorded 7
  distinct values drifting up *and* down:

  ```
  0xE0 → 0xD0 → 0xC2 → 0xBE → 0xBA → 0xAE → 0xB0
  (decimal: 224, 208, 194, 190, 186, 174, 176)
  ```

  The value can sit on a single setting for many minutes (the `0xE0` run was
  ~11 minutes in that capture) then step by a few counts. A C&H CH-36AHU
  capture recorded much more rapid movement through 0x00..0xFF.

  A second PNW capture (109 C0 frames, 3m 43s) deliberately exercised four
  user-initiated state transitions (OFF → FAN → COOL @ 20°C → COOL @ 22°C)
  and byte 28 **did not move** — it stayed at `0xE0` through every
  transition, as did byte 27 (`0x00`) and byte 29 (`0x01`). So byte 28 is
  **not coupled to `operation_mode` or `target_temperature`** on the
  user-command timescale. Whatever drives it, HVAC setpoint controls are
  not an input. Best remaining hypotheses: an internal sensor reading
  (compressor discharge? evaporator pressure?), a defrost / protection
  countdown, or some non-user-facing controller state.
  A separate pair of heat-mode captures also held bytes 28-29 steady at
  `0xE0/0x01` while byte 19 toggled between compressor running (`0x01`) and
  idle (`0x00`), so byte 28 is also **not coupled to compressor start/stop**
  on the observed timescale.
- **Byte 29 (`Unknown6`)** — hardware-dependent steady state: `0x01` across
  all 771 PNW frames; alternates `0x00`/`0x01` with occasional `0x02` on the
  C&H unit. Meaning unclear.

Any of these interpretations is provisional until more hardware is captured.

#### MidATRIX cross-reference (May 2026)

MidATRIX (author of [midea-s1s2-rs485-monitor](https://github.com/MidATRIX/midea-s1s2-rs485-monitor))
established an XYE tap on the same KJR-120W/MBF wired controller and posted two
candidate interpretations of C0 bytes 28-29 — they don't fully agree with each
other and neither has been confirmed on XYE:

1. **EEV position (16-bit LE)** — ["Reverse engineering Senville/Midea
   SComms"](https://community.home-assistant.io/t/reverse-engineering-senville-midea-scomms/992233/18):
   `c0_b28 = EEV Low`, `c0_b29 = EEV High`. Combined as `(b29 << 8) | b28`,
   little-endian. This is consistent with the IDU EEV step-count range and would
   match S1/S2's separate ODU `EXV_Position_Steps` (frame `0001_53` bytes 11+12)
   reported by the same project.

2. **Oil Return Cycle counter (single byte 28)** — ["Midea A/C via local
   XYE"](https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679/239),
   May 14: byte 28 ("0x1C / field 29" in MidATRIX's numbering) counts up to ~30
   min, then transitions into an oil-return percentage; when the percentage
   exceeds 90% (bits 0-6) the unit equalizes for the remaining ~2 min then
   resets. Bit 7 (`0x80`) is described as an "Active Oil Returning To Sump
   Cycle" flag. Observed in cool mode; defrost behaviour unknown.

**Our PNW capture fits interpretation 1 (EEV) more cleanly than interpretation
2 (oil return).** As a 16-bit LE EEV step count, the seven values become:

  ```
  (0x01 << 8) | 0xE0 = 0x01E0 = 480
  (0x01 << 8) | 0xD0 = 0x01D0 = 464
  (0x01 << 8) | 0xC2 = 0x01C2 = 450
  (0x01 << 8) | 0xBE = 0x01BE = 446
  (0x01 << 8) | 0xBA = 0x01BA = 442
  (0x01 << 8) | 0xAE = 0x01AE = 430
  (0x01 << 8) | 0xB0 = 0x01B0 = 432
  ```

  Tight 430-480 step range on an idle unit — plausible for an IDU EEV holding
  position with minor superheat-driven trim. By contrast, the oil-return
  interpretation requires bit 7 to be set on **all** seven observed values
  (it is — every value `≥ 0x80`), which would mean the unit was in "active
  oil returning to sump" for the entire 22-minute capture rather than counting
  up over 30 min and only triggering for ~2 min.

**Both are unconfirmed on XYE.** Resolving the conflict requires either:

- A simultaneous XYE + S1/S2 capture on the same unit (MidATRIX has both taps —
  ideal cross-validation source), or
- Logging XYE byte 28/29 over a full known compressor cycle on this project's
  hardware to see whether the values track a 0-500 EEV-step pattern or a
  30-min count-up-then-oil-return pattern.

Until then, treat these as **research hypotheses**, not decoded fields.

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

**Important — AUTO mode and the indoor unit**: The indoor unit hardware **never** reports AUTO in its C0 response. When the user selects AUTO (heat/cool as needed), the thermostat (CCM/ESPHome) is responsible for comparing the room temperature to the setpoint and switching between HEAT and COOL commands. The AC itself always receives an explicit HEAT or COOL command and reports back that mode. A software "smart thermostat" layer (e.g. the [HASS Smart Climate](https://github.com/HomeOps/HASS-Smart-Climate) component) is required to implement the AUTO logic in Home Assistant.

## Fan Modes

```
Mode        Value   Description                 Notes
----        -----   -----------                 -----
FAN_OFF     0x00    Fan off                     
FAN_HIGH    0x01    High speed                  
FAN_MEDIUM  0x02    Medium speed                
FAN_LOW_ALT 0x03    Low speed (alternate)       Observed in some implementations
FAN_LOW     0x04    Low speed                   
FAN_AUTO    0x80    Automatic fan speed         Bit 7 set; lower nibble may encode current physical speed (see below)
```

**Note**: Low fan speed has been observed as both FAN_LOW_ALT (0x03) and FAN_LOW (0x04) in different implementations. Use the value that works with your specific unit.

### Fan Mode Bitmask (received from AC)

The fan mode byte returned by the AC in the C0 response is a **bitmask**:

- **Bit 7 (`0x80`)** — AUTO fan flag: set when the unit is controlling fan speed automatically.
- **Lower nibble (`0x0F`)** — Current physical speed the fan is actually running at (uses the same FAN_HIGH/MEDIUM/LOW encoding above).

When the unit is in AUTO fan mode it reports combined values:

| Value  | Meaning                              |
|--------|--------------------------------------|
| `0x80` | Auto fan, no current speed info      |
| `0x81` | Auto fan, currently running at High  |
| `0x82` | Auto fan, currently running at Medium|
| `0x84` | Auto fan, currently running at Low   |

To decode:
- Check bit 7 (`value & 0x80`) to determine if the unit is in auto-fan mode.
- Mask the lower nibble (`value & 0x0F`) to get the current physical speed.

In this implementation `FAN_AUTO_FLAG = 0x80` and `FAN_SPEED_MASK = 0x0F` are defined as named constants.

## Temperature Encoding

### Celsius Encoding (Default)
```
encoded_value = (celsius * 2.0) + 0x28
celsius = (encoded_value - 0x28) / 2.0
```

**Note**: An alternative formulation (`(celsius / 0.5) + 0x30`) appears in the codeberg.org/xye/xye reference. This is **incorrect** — `0x30 = 48` while `0x28 = 40`, so the codeberg formula produces values that are 8 raw counts (4 °C) too high. Always use the `+ 0x28` formula.

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

Known protection-flag bits:

```
Bit / Mask      Name                Description
----------      ----                -----------
0x0002          DEFROST             Unit is currently running a defrost cycle
                                    (refrigeration cycle reversed; fan may keep
                                    running while no useful heating is delivered).
                                    Decoded as DEFROST_PROTECT_FLAG and surfaced via
                                    the optional `defrost:` binary sensor.
```

The exact meaning of the remaining bits varies by unit model and requires the service
manual.

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
STOP                0x04    Follow-Me stop / end-of-sequence marker (present in static pressure frames)
INIT                0x06    Initialization after mode change
```

### Static Pressure (ESP Profile) Frames

To set the external static pressure profile, byte 8 of the C6 frame encodes the profile level in the **lower nibble**, with bit 4 (`0x10`) always set:

```
Byte 8  Profile   Description
------  -------   -----------
0x10    SP0       Lowest static pressure profile
0x11    SP1
0x12    SP2
0x13    SP3
0x14    SP4       Highest static pressure profile
```

Byte 10 in these frames contains `0x04` (STOP), which is the Follow-Me stop flag — not a separate "static pressure subcommand".

### Emergency Heat Frame

To activate aux/emergency heat (backup heat strip only, compressor bypassed), send a C6 frame with **byte 8 = `0x80`** immediately after the C3 SET command.

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
- Some legacy/third-party protocol documentation (and some early tables) describe a direction flag bit, with 0x80 used for frames sent from the master and 0x00 for frames from the AC
- Actual on-wire traffic for XYE/CCM units shows byte 2 as 0x00 for both requests (master → AC) and responses (AC → master)
- This implementation always transmits 0x00 in byte 2 and expects 0x00 in responses, matching observed AC behavior
- Any references to 0x80 in direction-flag fields elsewhere in this document should be interpreted as legacy/spec behavior, not what this component actually sends on the wire

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

## Related Protocols — S1/S2 bus (IDU↔ODU)

A separate Midea RS-485 bus, the **S1/S2 bus**, runs between the indoor unit and the
outdoor inverter unit. It is a *different* bus from XYE/CCM (which sits between the IDU
and the wired thermostat / centralized controller), but it is part of the same
Midea protocol family and is **physically present on most Midea-based systems
simultaneously with the XYE bus**.

The [MidATRIX/midea-s1s2-rs485-monitor](https://github.com/MidATRIX/midea-s1s2-rs485-monitor)
project has reverse-engineered a substantial portion of S1/S2 and is the most useful
external cross-reference we have for narrowing down still-unknown XYE bytes (byte 15
`Current`, byte 16 `Unknown2`, bytes 27-29 `Unknown4/5/6` — see [Receive
Messages](#receive-messages-server--client) and [Byte 27-29
observations](#byte-27-29-observations)).

> ⚠️ **Bus voltage safety.** MidATRIX explicitly warns that S1/S2 bus voltage varies
> by system topology — their reference unit (separate mains supplies for IDU and ODU)
> reads 5 V, but mini-splits with a shared supply can present mains-level potential on
> the same terminals. **Always measure before tapping.**

### Framing differences (not directly comparable)

The two protocols share only the wire-level parameters; the frame layout, CRC, and
addressing are independent. Do **not** apply S1/S2 decoders to XYE bytes (or vice
versa) without verifying byte offsets first.

| Aspect             | XYE/CCM (this project)                | S1/S2 (MidATRIX)                                  |
| ------------------ | ------------------------------------- | ------------------------------------------------- |
| Bus role           | IDU ↔ CCM / wired thermostat          | IDU ↔ ODU (outdoor inverter)                      |
| UART               | 4800 8N1 half-duplex                  | 4800 8N1 half-duplex **(same)**                   |
| Preamble           | `0xAA`                                | `0xA0`                                            |
| Prologue           | `0x55`                                | (none — last 2 bytes are CRC)                     |
| Checksum           | 1-byte: `0xFF − sum(bytes 0…N-1)`     | CRC-16/MODBUS, little-endian                      |
| Frame length       | Fixed (16 TX / 32 RX)                 | Variable; length byte at offset 4                 |
| Address            | 1-byte device ID `0x00..0x3F`         | 2-byte device address (`0x0001`=ODU, `0x0100`=IDU)|
| Polling            | CCM master polls 64 IDs (130 ms slot) | ODU master, 24-frame cycle (~3.6 s)               |

### Field-level evidence that *is* useful

Even though framing differs, Midea reuses sensor concepts and similar — but **not
identical** — scaling formulas across both buses. This is where MidATRIX adds value:

| Concept                  | XYE (this protocol)                                  | S1/S2 equivalent (MidATRIX)                              | What we can infer                                                                                                                                                              |
| ------------------------ | ---------------------------------------------------- | -------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Operation mode enum      | bit-encoded: `0x80` AUTO, `0x88` COOL, `0x84` HEAT…  | raw enum: `0x00=Off, 0x01=Cool, 0x02=Heat, 0x03=Fan, 0x04=Dry, 0x07=Defrost` | XYE has no `DEFROST` value in `operation_mode` itself — instead defrost is exposed as bit `0x0002` of the 16-bit `protect_flags` field (bytes 24–25, see [Error and Protection Flags](#error-and-protection-flags)). This component already decodes it as `DEFROST_PROTECT_FLAG` and surfaces it via the optional `defrost:` binary sensor. S1/S2 lifts it to a top-level mode value; XYE keeps it as a protection-flag bit. |
| Fan enum                 | `0x01=H, 0x02=M, 0x03|0x04=L, 0x80=AUTO`             | `0x01=H, 0x02=M, 0x03=L, 0x06=Boost, 0x0F=Auto`          | Encoding diverges across buses — `0x06=Boost` and `0x0F=Auto` on S1/S2 do **not** apply to XYE. Use only the XYE encoding for XYE bytes.                                       |
| Temperature              | `(raw − 0x28) / 2` (offset 40)                       | `(raw − 61) / 2` or `(raw − 62) / 2` (offsets 61/62)     | Same `÷ 2` quantisation (0.5 °C steps); different bias. Useful pattern when investigating other XYE bytes that might encode temperature — try the `(raw − k) / 2` family first. |
| Compressor current       | byte 15 `Current` (often `0xFF`)                     | frame `0001_20` byte 12: `Compressor_Actual_Amps = raw / 3.2` | Current is an **ODU-side** quantity. The XYE `0xFF` is almost certainly an unimplemented-on-IDU sentinel, not a bug — IDU doesn't measure compressor current locally; the ODU does, and only S1/S2 sees it. |
| Demand / target frequency| not present                                          | IDU frame byte 7 `IDU_Demand_Hz` (0–96 Hz)                | XYE → CCM exchange doesn't carry demand-Hz; that handshake lives entirely on S1/S2. Don't look for it in XYE bytes.                                                            |
| EEV/EXV                  | byte 17 `IDU_EEV_Zone_Cmd` (zone command); **candidate**: bytes 28+29 may be IDU EEV step count (LE 16-bit) — see [Byte 27-29 observations](#byte-27-29-observations) | frame `0001_53` bytes 11+12 `EXV_Position_Steps` (LE 16-bit, 75–4200 steps) | XYE byte 17 is a **zone-level** command (Low/Med/High). MidATRIX has separately suggested XYE bytes 28+29 carry the **IDU EEV** position as a 16-bit LE step count; our PNW idle capture decodes to 430-480 steps under that hypothesis, which is physically plausible for an idle indoor EEV. Not yet confirmed. The ODU EXV position remains S1/S2-only. |
| Outdoor temp             | byte 14 `T3 Temperature` (one byte)                  | frame `0001_20` byte 10 `(raw·0.36775 − 17.2)` + byte 15 fractional `raw/696.125` | S1/S2 uses a 2-byte composite for ¼-°C precision; XYE's single byte is coarser. If a future model exposes a second outdoor-temp byte over XYE, the S1/S2 composite pattern is a candidate.|

### Candidate interpretations for XYE bytes 28 & 29

[Byte 27-29 observations](#byte-27-29-observations) show byte 28 drifting slowly while idle
(`0xE0 → 0xD0 → 0xC2 → 0xBE → 0xBA → 0xAE → 0xB0` over ~22 min on one PNW unit), byte 29
steady at `0x01`, and both staying steady through user mode/temperature/fan changes.

The current best candidates come from MidATRIX's May 2026 community-thread posts (see
[MidATRIX cross-reference](#midatrix-cross-reference-may-2026) in Byte 27-29 observations
for the full discussion and the side-by-side decode against our PNW capture):

- ✅ **IDU EEV position (16-bit LE across bytes 28+29)** — **current leading hypothesis.**
  Our PNW values decode to 430–480 steps under this interpretation, which is tight,
  monotonic-ish, and physically plausible for an idle IDU EEV. Cited in MidATRIX's
  Senville/Midea SComms thread (post #18).
- ⚠️ **Oil Return Cycle counter (single byte 28)** — alternative interpretation
  posted by MidATRIX in the XYE thread (post #239 area). Encoding: bits 0-6 =
  timer/percentage; bit 7 = "Active Oil Returning To Sump Cycle" flag. Cycle:
  ~30 min count-up → percentage; > 90% → ~2 min equalize → reset. Doesn't fit our
  PNW capture cleanly (bit 7 set on every value would imply the unit was in active
  oil return for the full 22 min) but may apply on other firmware/modes.

Earlier hypotheses (AC input voltage, heatsink/discharge temperature, runtime
counter) are superseded by the MidATRIX interpretations above and have been
retired. They didn't account for byte 29 staying steady at `0x01` — under the EEV
hypothesis that's simply the high byte of an EEV step count below 512.

**Both remaining hypotheses are unconfirmed on XYE.** To disambiguate, capture XYE
bytes 28-29 simultaneously with either (a) MidATRIX's S1/S2 frame `0001_53`
bytes 11+12 (EXV step count) on the same unit, or (b) a known full compressor +
oil-return cycle over ≥45 min. Open an issue with the capture if you run either.

### Where to look in the MidATRIX repo

If you want to dig further:

- [`src/decode/sensors.py`](https://github.com/MidATRIX/midea-s1s2-rs485-monitor/blob/main/src/decode/sensors.py) — concrete decoding for every frame ID, with exact byte offsets and scaling formulas.
- [`src/protocol/validator.py`](https://github.com/MidATRIX/midea-s1s2-rs485-monitor/blob/main/src/protocol/validator.py) — CRC-16/MODBUS implementation (relevant if you ever capture mixed-bus traffic).
- [README "Sensor Reference" section](https://github.com/MidATRIX/midea-s1s2-rs485-monitor#sensor-reference) — confidence-graded field table, including ⚠️ "probable" and ❓ "unknown" entries that themselves are still open research questions.
- [HA Community thread on Senville/Midea S-Comms](https://community.home-assistant.io/t/reverse-engineering-senville-midea-scomms/992233) — corresponding discussion forum.

## Other buses on the same IDU — HA/HB

Some Midea IDUs expose a **second** thermostat-bus terminal pair labelled **HA/HB**
(also seen as H1/H2 or HBL on adjacent series) in addition to X/Y/E. This pair is
used to connect Midea's premium wired thermostat, which gets a higher-bandwidth and
more privileged channel into the IDU than XYE/CCM offers — notably, the proprietary
thermostat can drive features such as native AUTO mode that XYE-connected controllers
cannot.

HA/HB is currently out of scope for this component. It is documented here so future
contributors don't confuse it with XYE or assume the same interface hardware works.

### ⚠️ CRITICAL: HA/HB backfeeds the XYE port at ≥19 V on KJR-120W/MBF

MidATRIX confirmed in [HA Community thread post
#239](https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679/239) (May
2026, while tapping a **KJR-120W/MBF** wired controller) that:

> "Do not connect HAHB and XYE from your wired controller to the IDU at the same
> time. When HAHB is connected and powered the XYE port pushes out 19V+. The same
> as HAHB. Don't blow up your boards…"

In other words, on this controller the **HA/HB rail backfeeds into the XYE
terminals through the controller's internal bus circuitry**. Connecting an XYE
TTL-level RS-485 dongle while HA/HB is still wired and energised will apply ~19 V
to nominally 5 V signal lines and **destroy the transceiver and possibly the host
MCU**.

MidATRIX's working configuration:

1. **Disconnect** HA/HB from the IDU/controller before wiring up XYE.
2. Provide a **dedicated 12 V supply** to the XYE `+` rail and ground to `E`.
3. With HA/HB de-energised, the wired controller communicates over XYE at the
   expected **5 V** logic levels and **4800 8N1** (same baud as S1/S2 — confirmed
   in the same post).

Trade-off observed: connecting the wired controller to XYE caused the Senville
app to lose access to extended-query data (current power usage, blower CFM) —
consistent with bus arbitration handing those polls to the wired controller
instead of the app's gateway.

### Electrical layer — not standard RS-485

A voltmeter reading across an idle HA/HB pair on one reporter's unit measures
**~18.5 VDC**, consistent with MidATRIX's "19 V+" observation on the
backfed XYE port of the same controller family. That voltage is **outside the
standard RS-485 common-mode range** (−7 V to +12 V; some "extended" parts go to
±25 V), so HA/HB is almost certainly not plain RS-485. Plugging this project's
XYE RS-485 dongle into HA/HB will at best log garbage and at worst destroy the
transceiver.

Likely PHY options (need a scope capture to disambiguate):

- **Single-ended load modulation** — bus rests at ~18 V; the talker sinks current to
  pull it low for each bit. Common on long-run HVAC thermostat wiring because the
  same pair powers the thermostat.
- **Low-amplitude differential AC-coupled on a DC bias** — 18 V is just power; a
  ±0.5–1 V differential signal rides on top. Requires an isolated extended-common-
  mode transceiver (e.g. ADM2582E, ISO1500, MAX22500E) to sniff.
- **Current loop / Modbus-over-current variant** — bus voltage dips during bit
  transitions rather than swinging cleanly.

### Relationship to S1/S2 and XYE

Functionally HA/HB is **IDU ↔ thermostat**, not IDU ↔ ODU — so it is *not* the same
bus as S1/S2 (which MidATRIX documents). But because Midea reuses internal
high-bandwidth protocols across product lines, the **message-layer frame format**
(`0xA0` preamble, length-prefixed payload, CRC-16/MODBUS — see [Framing
differences](#framing-differences-not-directly-comparable)) is a reasonable starting
guess once you have HA/HB bytes off the wire. The endpoints (thermostat address vs.
ODU address) and message IDs will differ.

If anyone captures HA/HB traffic and validates this guess, please open an issue —
no public reverse-engineering of the Midea premium-thermostat bus exists today, and
that gap is the most likely explanation for what the proprietary thermostat does
that an XYE-connected controller can't.

### Investigation checklist

Before guessing the protocol, characterise the PHY:

1. Confirm the model number and check the IDU wiring diagram (often inside the
   front cover) — the diagram identifies each terminal pair and may name HA/HB
   explicitly.
2. Scope the line: any USB scope (Hantek 6022BE / Owon / Analog Discovery) AC-coupled
   on one wire referenced to chassis GND, ~2 V/div, capture during a known thermostat
   command (power on/off, change setpoint). The waveform identifies the PHY:
   - Clean ±2.5 V symmetric swing on 18 V DC → AC-coupled differential.
   - Bus dropping 18 V → ~0 V in bit-shaped pulses → single-ended pulldown.
   - Asymmetric rise/fall → load modulation.
3. Once the PHY is known, build the interface accordingly:
   - Differential → isolated extended-common-mode RS-485 transceiver.
   - Single-ended pulldown → optocoupler + comparator.
   - Load modulation → custom analog front-end.
4. With clean bytes on a serial port, try MidATRIX's frame validator first
   (`0xA0` preamble, CRC-16/MODBUS). If frames pass CRC, the message-layer protocol
   is shared with S1/S2 and reuse of their decoder framework becomes practical.

## References

1. **XYE Reverse Engineering Project**
   - https://codeberg.org/xye/xye
   - Original reverse engineering work on the protocol
   - **Note**: The temperature encoding formula in this reference (`+0x30`) is incorrect. Use `+0x28` (see Temperature Encoding section).

2. **wtahler's Implementation**
   - https://github.com/wtahler/esphome-mideaXYE-rs485
   - Working ESP implementation with field mappings

3. **Bunicutz's ESP32 Implementation**
   - https://github.com/Bunicutz/ESP32_Midea_RS485
   - Another working implementation with insights

4. **ESPHome Midea Component**
   - https://github.com/esphome/esphome/tree/dev/esphome/components/midea
   - Related Midea protocol (different from XYE)

5. **HA Community Thread — Midea A/C via local XYE**
   - https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679
   - [Archived PDF](https://github.com/user-attachments/files/26555742/Midea.A_C.via.local.XYE.-.ESPHome.-.Home.Assistant.Community.pdf)
   - Extended protocol analysis including fan mode bitmask, static pressure frames, emergency heat, C4 field decoding, and AUTO mode behaviour. Contributors: mdrobnak, rymo, and others.

6. **mdrobnak's units_switch branch**
   - https://github.com/mdrobnak/esphome/tree/units_switch
   - Source of Fahrenheit switch, defrost sensor, and fan speed text sensor features

7. **MidATRIX — midea-s1s2-rs485-monitor**
   - https://github.com/MidATRIX/midea-s1s2-rs485-monitor
   - Companion reverse-engineering of the Midea **S1/S2 bus** (IDU↔ODU) — a
     different bus from XYE/CCM but the same protocol family. Provides field-level
     evidence (sensor inventory, scaling formulas, defrost/EXV/compressor signals)
     useful for narrowing down still-unknown XYE bytes. See [Related Protocols](#related-protocols--s1s2-bus-iduodu).

8. **MidATRIX community posts on XYE/HA-HB/oil-return cycle (May 2026)**
   - HA Community ["Midea A/C via local XYE" post #239](https://community.home-assistant.io/t/midea-a-c-via-local-xye/857679/239) — KJR-120W/MBF wiring breakthrough, HA/HB → XYE port 19 V backfeed warning, confirmation that XYE and S1/S2 both run 4800 8N1, observation that wired-controller-on-XYE blocks Senville-app extended queries.
   - HA Community ["Reverse engineering Senville/Midea SComms" post #18](https://community.home-assistant.io/t/reverse-engineering-senville-midea-scomms/992233/18) — `c0_b28 = EEV Low`, `c0_b29 = EEV High` (IDU EEV step count as 16-bit LE) and `c0_b19` active during the 2-min oil-return process.
   - Same community thread, follow-up post: alternative interpretation of byte 28 as an Oil Return Cycle counter (bits 0-6 timer/percentage, bit 7 active flag, ~30 min cycle, 90 % threshold, 2 min equalize). Conflicts with the EEV-position interpretation; both documented in [Byte 27-29 observations](#byte-27-29-observations).

## Version History

- **v1.2** (2026-06-15): Incorporated MidATRIX May 2026 community posts: dual candidate interpretations for C0 bytes 28-29 (IDU EEV position 16-bit LE vs Oil Return Cycle counter), critical HA/HB→XYE 19 V backfeed safety warning on KJR-120W/MBF, retired the obsolete AC-voltage and temperature byte-28 hypotheses.
- **v1.1** (2026-05-24): Added "Related Protocols — S1/S2 bus" section with MidATRIX cross-reference and byte-28 hypotheses
- **v1.0** (2026-01-30): Initial documentation based on code analysis and external references
