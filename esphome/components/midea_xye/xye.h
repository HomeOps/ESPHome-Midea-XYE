#pragma once

#ifdef USE_ARDUINO

#include <cstdint>
#include <map>
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

// Type aliases for node identifiers and measurement values
using NodeId = uint8_t;  ///< Node ID type for server and client identifiers (0x00..0x3F, 0xFF for broadcast)
using FanPwm = uint8_t;  ///< Fan PWM duty cycle (0-255, 0x00 = off, 0xFF = max)
using FanTach = uint8_t; ///< Fan tachometer feedback value (0x00 = not available on some models)
using ValvePosition = uint8_t; ///< Expansion valve position (0-255, 0x00 = closed/not available)

/**
 * @brief Protocol framing markers
 */
enum class ProtocolMarker : uint8_t {
  PREAMBLE = 0xAA,  ///< Start byte for all messages
  PROLOGUE = 0x55   ///< End byte for all messages
};

// Legacy compatibility constants
constexpr uint8_t PROTOCOL_PREAMBLE = static_cast<uint8_t>(ProtocolMarker::PREAMBLE);
constexpr uint8_t PROTOCOL_PROLOGUE = static_cast<uint8_t>(ProtocolMarker::PROLOGUE);

// Message lengths
constexpr uint8_t TX_MESSAGE_LENGTH = 16;  ///< Length of transmitted messages
constexpr uint8_t RX_MESSAGE_LENGTH = 32;  ///< Length of received messages

/**
 * @brief Message direction indicators
 * 
 * Note: In actual AC implementations, both directions use 0x00 in the direction byte.
 * Messages are distinguished by context (who initiated the communication) and command type,
 * not by the direction byte value. This matches v0.0.10 behavior and actual AC units.
 */
enum class Direction : uint8_t {
  FROM_CLIENT = 0x00,  ///< Message from client (thermostat) to server (HVAC)
  TO_CLIENT = 0x00     ///< Message from server (HVAC) to client (thermostat). Set to 0x00 to match actual AC behavior.
};

// Legacy compatibility
constexpr uint8_t FROM_CLIENT = static_cast<uint8_t>(Direction::FROM_CLIENT);
constexpr uint8_t TO_CLIENT = static_cast<uint8_t>(Direction::TO_CLIENT);

// Node ID constants
constexpr NodeId SERVER_ID = 0x00;        ///< ID of the HVAC unit (server) - typical default
constexpr NodeId CLIENT_ID = 0x00;        ///< ID of the thermostat (client/master) - typical default
constexpr NodeId BROADCAST_ID = 0xFF;     ///< Broadcast ID - messages sent to all units
constexpr NodeId MAX_DEVICE_ID = 0x3F;    ///< Maximum valid device ID (0x00..0x3F)

/**
 * @brief Command types sent from client (thermostat) to server (HVAC unit)
 *
 * Note: Server responses use the same command codes, so there's no separate
 * ServerCommand enum in this protocol variant.
 */
enum class Command : uint8_t {
  QUERY = 0xC0,                                    ///< Query current status (basic)
  QUERY_EXTENDED = 0xC4,                           ///< Query extended status - derived from QUERY (0xC0 | 0x04)
  SET = 0xC3,                                      ///< Set operation parameters - derived from QUERY (0xC0 | 0x03)
  FOLLOW_ME = 0xC6,                                ///< Follow-Me temperature update - derived from QUERY (0xC0 | 0x06)
  LOCK = 0xCC,                                     ///< Lock the physical controls - derived from QUERY (0xC0 | 0x0C)
  UNLOCK = 0xCD,                                   ///< Unlock the physical controls - derived from QUERY (0xC0 | 0x0D)
};

/**
 * @brief Operation modes for the HVAC unit
 * 
 * Note: Some implementations have observed AUTO mode as 0x91 (0x80 | 0x10 | 0x01).
 * The exact value may vary by unit model. This implementation uses 0x80.
 * See PROTOCOL.md for details on protocol variations.
 */
enum class OperationMode : uint8_t {
  OFF = 0x00,        ///< Unit is off
  AUTO = 0x80,       ///< Automatic mode (heat/cool as needed)
  FAN = 0x81,        ///< Fan only mode - derived from AUTO (0x80 | 0x01)
  DRY = 0x82,        ///< Dehumidify mode - derived from AUTO (0x80 | 0x02)
  HEAT = 0x84,       ///< Heating mode - derived from AUTO (0x80 | 0x04)
  COOL = 0x88,       ///< Cooling mode - derived from AUTO (0x80 | 0x08)
  AUTO_ALT = 0x91    ///< Alternate AUTO mode (0x80 | 0x10 | 0x01) - observed in some implementations
};

/**
 * @brief Fan speed modes
 * 
 * Note: Names avoid conflicts with Arduino HIGH/LOW macros.
 * Some implementations have observed LOW fan as 0x03 instead of 0x04.
 * The exact value may vary by unit model. This implementation uses 0x04.
 * See PROTOCOL.md for details on protocol variations.
 * 
 * When receiving from the AC unit, the fan mode byte is a bitmask:
 *   - Bit 7 (FAN_AUTO_FLAG = 0x80): AUTO fan mode — the unit is controlling speed automatically.
 *   - Lower nibble (FAN_SPEED_MASK = 0x0F): Current physical speed (FAN_HIGH/MEDIUM/LOW/OFF).
 * Combined values such as 0x81 (AUTO+HIGH), 0x82 (AUTO+MEDIUM), 0x84 (AUTO+LOW) are normal.
 * Check bit 7 for AUTO status; mask with FAN_SPEED_MASK for the actual running speed.
 */
enum class FanMode : uint8_t {
  FAN_OFF = 0x00,     ///< Fan off
  FAN_HIGH = 0x01,    ///< High speed
  FAN_MEDIUM = 0x02,  ///< Medium speed
  FAN_LOW_ALT = 0x03, ///< Low speed (alternate) - observed in some implementations
  FAN_LOW = 0x04,     ///< Low speed
  FAN_AUTO = 0x80     ///< Automatic fan speed (bit 7 set; lower nibble may encode current physical speed)
};

/**
 * @brief Bit 7 of the fan mode byte: indicates the unit is in AUTO fan mode.
 * When this bit is set the lower nibble (FAN_SPEED_MASK) encodes the current physical speed.
 */
constexpr uint8_t FAN_AUTO_FLAG = 0x80;

/**
 * @brief Mask to extract the current physical fan speed from the fan mode byte.
 * Apply this mask before comparing to FanMode enum values (FAN_HIGH/MEDIUM/LOW/OFF).
 */
constexpr uint8_t FAN_SPEED_MASK = 0x0F;

/**
 * @brief Flags for special operation modes
 */
enum class ModeFlags : uint8_t {
  NORMAL = 0x00,      ///< Normal operation
  ECO = 0x01,         ///< Energy saving mode
  AUX_HEAT = 0x02,    ///< Auxiliary heat / Turbo mode
  SWING = 0x04,       ///< Swing mode enabled
  VENTILATION = 0x88  ///< Ventilation mode
};

/**
 * @brief Operation flags from unit status
 */
enum class OperationFlags : uint8_t {
  WATER_PUMP = 0x04,  ///< Water pump active
  WATER_LOCK = 0x80   ///< Water lock protection active
};

/**
 * @brief Unit capabilities flags
 */
enum class Capabilities : uint8_t {
  EXTERNAL_TEMP = 0x80,  ///< Supports external temperature sensor
  SWING = 0x10           ///< Supports swing function
};

/**
 * @brief CCM communication error flags
 */
enum class CcmErrorFlags : uint8_t {
  NO_ERROR = 0x00,        ///< No communication errors
  TIMEOUT = 0x01,         ///< Communication timeout
  CRC_ERROR = 0x02,       ///< CRC check failed
  PROTOCOL_ERROR = 0x04   ///< Protocol violation
};

/**
 * @brief Auto mode flag in operation mode byte
 */
constexpr uint8_t OP_MODE_AUTO_FLAG = 0x10;

/**
 * @brief Mask to extract the operation mode value, clearing the auto-mode flag bit.
 * Clears OP_MODE_AUTO_FLAG (0x10) while preserving all other bits.
 */
constexpr uint8_t OP_MODE_VALUE_MASK = static_cast<uint8_t>(~OP_MODE_AUTO_FLAG);  ///< 0xEF

/**
 * @brief Flag bit set in the target temperature byte of a Follow-Me static pressure message.
 * Bit 4 (0x10) signals to the unit that the lower nibble carries a static pressure value.
 */
constexpr uint8_t STATIC_PRESSURE_FLAG = 0x10;

/**
 * @brief Mask to extract the 4-bit static pressure value from its byte.
 */
constexpr uint8_t STATIC_PRESSURE_VALUE_MASK = 0x0F;

/**
 * @brief Engineering offset for Fahrenheit temperatures in the XYE protocol.
 * The unit stores Fahrenheit setpoints as: encoded = (int)fahrenheit + FAHRENHEIT_TEMP_OFFSET.
 * To decode: fahrenheit = encoded - FAHRENHEIT_TEMP_OFFSET.
 */
constexpr uint8_t FAHRENHEIT_TEMP_OFFSET = 0x87;

/**
 * @brief Timer duration flags (combinable)
 */
enum class TimerFlags : uint8_t {
  TIMER_15MIN = 0x01,   ///< 15 minute increment
  TIMER_30MIN = 0x02,   ///< 30 minute increment
  TIMER_1HOUR = 0x04,   ///< 1 hour increment
  TIMER_2HOUR = 0x08,   ///< 2 hour increment
  TIMER_4HOUR = 0x10,   ///< 4 hour increment
  TIMER_8HOUR = 0x20,   ///< 8 hour increment
  TIMER_16HOUR = 0x40,  ///< 16 hour increment
  INVALID = 0x80        ///< Timer not set/invalid
};

/**
 * @brief Follow-Me subcommand types (used in the timer_stop field of a Follow-Me transmit message)
 */
enum class FollowMeSubcommand : uint8_t {
  UPDATE = 0x02, ///< Regular temperature update
  STOP   = 0x04, ///< Follow-Me stop / end-of-sequence marker (also present in static pressure frames)
  INIT   = 0x06  ///< Initialization after mode change
};

/**
 * @brief Control state machine states
 */
enum class ControlState : uint8_t {
  WAIT_DATA = 0,            ///< Waiting for response from command
  SEND_SET = 1,             ///< Sending Set (0xC3) command
  SEND_FOLLOWME = 2,        ///< Sending Follow-Me (0xC6) command
  SEND_QUERY = 3,           ///< Sending Query (0xC0) command
  SEND_QUERY_EXTENDED = 4   ///< Sending Extended Query (0xC4) command
};

/**
 * @brief Response/status codes
 */
enum class ResponseCode : uint8_t {
  UNKNOWN = 0x00,
  OK = 0x30,
  UNKNOWN1 = 0xFF,
  UNKNOWN2 = 0x01,
  UNKNOWN3 = 0x00
};

/**
 * @brief Compressor status flags (C4 extended query)
 * 
 * This field is a bitmask where:
 * - Bit 7 (0x80): Compressor active/running
 * - Bits 0-6: Currently undefined/reserved in protocol documentation
 * 
 * The current implementation defines only the known protocol states:
 * - 0x00: Compressor idle (bit 7 clear)
 * - 0x80: Compressor active (bit 7 set, other bits clear)
 * 
 * If hardware sets other bits (e.g., 0x81, 0x82, 0xC0), enum_to_string will
 * return "UNKNOWN". To check compressor status, test bit 7 directly rather
 * than relying on exact enum match.
 */
enum class CompressorFlags : uint8_t {
  IDLE = 0x00,           ///< Compressor idle/not running (all bits clear)
  ACTIVE = 0x80          ///< Compressor active/running (bit 7 set, others clear)
};

/**
 * @brief ESP (External Static Pressure) profile settings (C4 extended query)
 * Controls airflow mode and ducted AHU fan table selection
 */
enum class EspProfile : uint8_t {
  ESP_LOW = 0x10,            ///< Low ESP profile
  ESP_MEDIUM = 0x30,         ///< Medium ESP profile (normal airflow curve)
  ESP_HIGH = 0x50            ///< High ESP profile
};

/**
 * @brief Protection and outdoor fan state flags (C4 extended query)
 * Bitmask indicating various protection states and outdoor fan operation
 */
enum class ProtectionFlags : uint8_t {
  NONE = 0x00,                  ///< No protections active, fan off
  OUTDOOR_FAN_RUNNING = 0x80,   ///< Outdoor fan running (bit 7)
  COMPRESSOR_ACTIVE = 0x8C      ///< Compressor active, outdoor fan running, no protections (0x80 | 0x0C)
};

/**
 * @brief System status flags (C4 extended query)
 * 
 * This field is a bitmask where:
 * - Bit 7 (0x80): System enabled
 * - Bit 2 (0x04): Wired controller present
 * - Other bits: Currently undefined/reserved
 * 
 * Enum values define common observed combinations. If hardware sets other
 * bit combinations (e.g., 0x81, 0x85), enum_to_string will return "UNKNOWN".
 * 
 * To check specific flags, test bits directly:
 * - (value & 0x80) for system enabled
 * - (value & 0x04) for wired controller present
 */
enum class SystemStatusFlags : uint8_t {
  SYSTEM_DISABLED = 0x00,             ///< System disabled, no controller
  WIRED_CONTROLLER = 0x04,            ///< Wired controller present, system disabled (bit 2)
  ENABLED = 0x80,                     ///< System enabled, no controller (bit 7)
  ENABLED_WITH_CONTROLLER = 0x84      ///< System enabled with wired controller (bits 2 and 7)
};

/**
 * @brief Subsystem protection/OK flags (C4 extended query)
 * Used for compressor, outdoor fan, 4-way valve, and inverter subsystems
 * 
 * Bit 7 (0x80) indicates subsystem OK status.
 * Other bit combinations not listed may indicate specific protection states.
 */
enum class SubsystemFlags : uint8_t {
  PROTECTION_ACTIVE = 0x00,  ///< Protection triggered or subsystem not OK
  OK = 0x80                  ///< Subsystem OK, no protection active (bit 7 set)
};

/// Defrost active flag within the 16-bit `protect_flags` field of the C0 QUERY response.
/// When set, the indoor unit is currently running a defrost cycle.
constexpr uint16_t DEFROST_PROTECT_FLAG = 0x0002;

/**
 * @brief Special temperature value for fan mode
 */
constexpr uint8_t TEMP_FAN_MODE = 0xFF;

/**
 * @brief Status flag bit in the target temperature byte (byte 10 of C0 response).
 *
 * Bit 6 (0x40) of the target temperature byte can be set by the unit to signal
 * an internal state (exact meaning unknown; observed when unit is in certain states).
 * It is unrelated to the temperature value and must be cleared before converting
 * the byte to a Celsius setpoint. Temperature is always transmitted as raw Celsius.
 */
constexpr uint8_t SET_TEMP_STATUS_FLAG = 0x40;

/**
 * @brief Mask to extract the actual temperature value from the target temperature byte.
 *
 * Clears SET_TEMP_STATUS_FLAG (0x40) while preserving all other bits.
 */
constexpr uint8_t SET_TEMP_VALUE_MASK = static_cast<uint8_t>(~SET_TEMP_STATUS_FLAG);  ///< 0xBF

/**
 * @brief Byte offset used in the XYE sensor-temperature encoding.
 *
 * Encoded formula (Celsius): encoded = (celsius * TEMP_ENCODING_SCALE) + TEMP_ENCODING_OFFSET
 * Decode formula:            celsius = (encoded - TEMP_ENCODING_OFFSET) / TEMP_ENCODING_SCALE
 *
 * Used by T1/T2a/T2b/T3/outdoor temperature bytes in C0 and C4 responses.
 */
constexpr uint8_t TEMP_ENCODING_OFFSET = 0x28;

/**
 * @brief Scale factor used in the XYE sensor-temperature encoding (2 counts per °C).
 */
constexpr float TEMP_ENCODING_SCALE = 2.0f;

/**
 * @brief Temperature encoding type
 */
enum class TemperatureEncoding : uint8_t {
  ENCODED = 0,  ///< Standard encoding: (celsius * TEMP_ENCODING_SCALE) + TEMP_ENCODING_OFFSET
  RAW = 1       ///< Raw Celsius value (no encoding)
};

/**
 * @brief Temperature value (encoded)
 * 
 * Temperature Encoding Formula (Celsius):
 *   encoded_value = (celsius * TEMP_ENCODING_SCALE) + TEMP_ENCODING_OFFSET
 *   celsius = (value - TEMP_ENCODING_OFFSET) / TEMP_ENCODING_SCALE
 * 
 * Example: 20°C → (20 * 2) + 0x28 = 0x50 (80 decimal)
 * 
 * Note: Some implementations use raw Fahrenheit values without encoding.
 * The behavior may depend on unit configuration or regional settings.
 * See PROTOCOL.md for details on temperature encoding variations.
 */
struct __attribute__((packed)) Temperature {
  uint8_t value;  ///< Encoded temperature value
  
  /// Convert to degrees Celsius
  float to_celsius() const;
  
  /// Create from degrees Celsius
  static Temperature from_celsius(float celsius);
  
  /// Print debug information
  /// @param tag Log tag
  /// @param name Field name
  /// @param left Bytes remaining to read
  /// @param level Log level
  /// @param encoding Temperature encoding type (ENCODED by default)
  /// @return Updated bytes remaining
  size_t print_debug(const char *tag, const char *name, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG, 
                     TemperatureEncoding encoding = TemperatureEncoding::ENCODED) const;
};

/**
 * @brief Direction and node ID pair (2 bytes)
 */
struct __attribute__((packed)) DirectionNode {
  Direction direction;  ///< Direction indicator
  NodeId node_id;       ///< Node identifier
  
  /// Print debug information
  /// @param tag Log tag
  /// @param name Field name
  /// @param left Bytes remaining to read
  /// @param level Log level
  /// @return Updated bytes remaining
  size_t print_debug(const char *tag, const char *name, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

/**
 * @brief 16-bit flag field split into low and high bytes
 */
struct __attribute__((packed)) Flags16 {
  uint8_t low;   ///< Low byte (bits 0-7)
  uint8_t high;  ///< High byte (bits 8-15)
  
  /// Get combined 16-bit value
  uint16_t value() const;
  
  /// Set from 16-bit value
  void set(uint16_t val);
  
  /// Print debug information
  /// @param tag Log tag
  /// @param name Field name
  /// @param left Bytes remaining to read
  /// @param level Log level
  /// @return Updated bytes remaining
  size_t print_debug(const char *tag, const char *name, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

/**
 * @brief 16-bit value stored in big-endian byte order (high byte first)
 * Used for engineering values where protocol transmits high byte before low byte
 */
struct __attribute__((packed)) Flags16BigEndian {
  uint8_t high;  ///< High byte (bits 8-15)
  uint8_t low;   ///< Low byte (bits 0-7)
  
  /// Get combined 16-bit value
  uint16_t value() const;
  
  /// Set from 16-bit value
  void set(uint16_t val);
  
  /// Print debug information
  /// @param tag Log tag
  /// @param name Field name
  /// @param left Bytes remaining to read
  /// @param level Log level
  /// @return Updated bytes remaining
  size_t print_debug(const char *tag, const char *name, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

/**
 * @brief Common message termination (CRC + prologue)
 * Size: 2 bytes
 */
struct __attribute__((packed)) MessageFrameEnd {
  uint8_t crc;                 ///< Checksum (CRC)
  ProtocolMarker prologue;     ///< Must be 0x55
};

// Forward declarations for enum maps
extern const std::map<Command, const char*> COMMAND_MAP;
extern const std::map<OperationMode, const char*> OPERATION_MODE_MAP;
extern const std::map<FanMode, const char*> FAN_MODE_MAP;
extern const std::map<ModeFlags, const char*> MODE_FLAGS_MAP;
extern const std::map<OperationFlags, const char*> OPERATION_FLAGS_MAP;
extern const std::map<Capabilities, const char*> CAPABILITIES_MAP;
extern const std::map<Direction, const char*> DIRECTION_MAP;
extern const std::map<CcmErrorFlags, const char*> CCM_ERROR_FLAGS_MAP;
extern const std::map<FollowMeSubcommand, const char*> FOLLOW_ME_SUBCOMMAND_MAP;
extern const std::map<CompressorFlags, const char*> COMPRESSOR_FLAGS_MAP;
extern const std::map<EspProfile, const char*> ESP_PROFILE_MAP;
extern const std::map<ProtectionFlags, const char*> PROTECTION_FLAGS_MAP;
extern const std::map<SystemStatusFlags, const char*> SYSTEM_STATUS_FLAGS_MAP;
extern const std::map<SubsystemFlags, const char*> SUBSYSTEM_FLAGS_MAP;

/**
 * @brief Trait struct to map enum types to their string mappings
 */
template<typename EnumType>
struct EnumTraits;

template<> struct EnumTraits<Command> { static const std::map<Command, const char*>& get_map() { return COMMAND_MAP; } };
template<> struct EnumTraits<OperationMode> { static const std::map<OperationMode, const char*>& get_map() { return OPERATION_MODE_MAP; } };
template<> struct EnumTraits<FanMode> { static const std::map<FanMode, const char*>& get_map() { return FAN_MODE_MAP; } };
template<> struct EnumTraits<ModeFlags> { static const std::map<ModeFlags, const char*>& get_map() { return MODE_FLAGS_MAP; } };
template<> struct EnumTraits<OperationFlags> { static const std::map<OperationFlags, const char*>& get_map() { return OPERATION_FLAGS_MAP; } };
template<> struct EnumTraits<Capabilities> { static const std::map<Capabilities, const char*>& get_map() { return CAPABILITIES_MAP; } };
template<> struct EnumTraits<Direction> { static const std::map<Direction, const char*>& get_map() { return DIRECTION_MAP; } };
template<> struct EnumTraits<CcmErrorFlags> { static const std::map<CcmErrorFlags, const char*>& get_map() { return CCM_ERROR_FLAGS_MAP; } };
template<> struct EnumTraits<FollowMeSubcommand> { static const std::map<FollowMeSubcommand, const char*>& get_map() { return FOLLOW_ME_SUBCOMMAND_MAP; } };
template<> struct EnumTraits<CompressorFlags> { static const std::map<CompressorFlags, const char*>& get_map() { return COMPRESSOR_FLAGS_MAP; } };
template<> struct EnumTraits<EspProfile> { static const std::map<EspProfile, const char*>& get_map() { return ESP_PROFILE_MAP; } };
template<> struct EnumTraits<ProtectionFlags> { static const std::map<ProtectionFlags, const char*>& get_map() { return PROTECTION_FLAGS_MAP; } };
template<> struct EnumTraits<SystemStatusFlags> { static const std::map<SystemStatusFlags, const char*>& get_map() { return SYSTEM_STATUS_FLAGS_MAP; } };
template<> struct EnumTraits<SubsystemFlags> { static const std::map<SubsystemFlags, const char*>& get_map() { return SUBSYSTEM_FLAGS_MAP; } };

/**
 * @brief Template function for enum-to-string conversion
 * Uses EnumTraits to automatically select the correct mapping for each enum type
 */
template<typename EnumType>
const char* enum_to_string(EnumType value) {
  const auto& mapping = EnumTraits<EnumType>::get_map();
  auto it = mapping.find(value);
  if (it != mapping.end()) {
    return it->second;
  }
  return "UNKNOWN";
}

/**
 * @brief Helper to print a uint8_t field with bounds checking
 * @param tag Log tag
 * @param name Field name
 * @param value The uint8_t value to print
 * @param left Bytes remaining
 * @param level Log level
 * @return Updated bytes remaining
 */
inline size_t print_debug_uint8(const char *tag, const char *name, uint8_t value, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) {
  if (left < 1) return left;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%02X"), name, value);
  return left - 1;
}

/**
 * @brief Helper to print an enum field with bounds checking
 * @param tag Log tag
 * @param name Field name  
 * @param value The enum value to print
 * @param left Bytes remaining
 * @param level Log level
 * @return Updated bytes remaining
 */
template<typename EnumType>
inline size_t print_debug_enum(const char *tag, const char *name, EnumType value, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) {
  if (left < 1) return left;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%02X (%s)"), 
           name, static_cast<uint8_t>(value), enum_to_string(value));
  return left - 1;
}

// Static assertions
static_assert(sizeof(ProtocolMarker) == 1, "ProtocolMarker must be 1 byte");
static_assert(sizeof(MessageFrameEnd) == 2, "MessageFrameEnd must be 2 bytes (CRC + prologue)");

/// Compute XYE protocol CRC.
///
/// Sums all bytes in the buffer except the byte at position @c len-2 (the CRC
/// slot itself).  Returns @c 0xFF minus the low byte of that sum.
///
/// @param data  Pointer to the start of the message buffer.
/// @param len   Total buffer length, including the CRC byte and the trailing
///              prologue byte.
/// @return      The computed CRC byte.
inline uint8_t compute_protocol_crc(const uint8_t *data, uint8_t len) noexcept {
  uint32_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    if (i != len - 2)
      crc += data[i];
  }
  return static_cast<uint8_t>(0xFF - (crc & 0xFF));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
