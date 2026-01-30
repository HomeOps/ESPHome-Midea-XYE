#pragma once

#ifdef USE_ARDUINO

#include <cstdint>
#include <map>
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

// Type aliases for node identifiers
using NodeId = uint8_t;  ///< Node ID type for server and client identifiers

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
 */
enum class Direction : uint8_t {
  FROM_CLIENT = 0x00,  ///< Message from client (thermostat) to server (HVAC)
  TO_CLIENT = 0x80     ///< Message from server (HVAC) to client (thermostat)
};

// Legacy compatibility
constexpr uint8_t FROM_CLIENT = static_cast<uint8_t>(Direction::FROM_CLIENT);
constexpr uint8_t TO_CLIENT = static_cast<uint8_t>(Direction::TO_CLIENT);

// TODO: Don't hardcode these IDs
constexpr NodeId SERVER_ID = 0;  ///< ID of the HVAC unit (server)
constexpr NodeId CLIENT_ID = 0;  ///< ID of the thermostat (client)

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
 */
enum class OperationMode : uint8_t {
  OFF = 0x00,        ///< Unit is off
  AUTO = 0x80,       ///< Automatic mode (heat/cool as needed)
  FAN = 0x81,        ///< Fan only mode - derived from AUTO (0x80 | 0x01)
  DRY = 0x82,        ///< Dehumidify mode - derived from AUTO (0x80 | 0x02)
  HEAT = 0x84,       ///< Heating mode - derived from AUTO (0x80 | 0x04)
  COOL = 0x88        ///< Cooling mode - derived from AUTO (0x80 | 0x08)
};

/**
 * @brief Fan speed modes
 * Note: Names avoid conflicts with Arduino HIGH/LOW macros
 */
enum class FanMode : uint8_t {
  FAN_OFF = 0x00,     ///< Fan off
  FAN_HIGH = 0x01,    ///< High speed
  FAN_MEDIUM = 0x02,  ///< Medium speed
  FAN_LOW = 0x04,     ///< Low speed
  FAN_AUTO = 0x80     ///< Automatic fan speed
};

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
 * @brief Follow-Me subcommand types (used in TXData[10])
 */
enum class FollowMeSubcommand : uint8_t {
  UPDATE = 0x02,          ///< Regular temperature update
  STATIC_PRESSURE = 0x04, ///< Static pressure setting
  INIT = 0x06             ///< Initialization after mode change
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
 * @brief Special temperature value for fan mode
 */
constexpr uint8_t TEMP_FAN_MODE = 0xFF;

/**
 * @brief Temperature value (encoded)
 * Formula: actual_temp = (value - 0x28) / 2.0
 */
struct __attribute__((packed)) Temperature {
  uint8_t value;  ///< Encoded temperature value
  
  /// Convert to degrees Celsius
  float to_celsius() const;
  
  /// Create from degrees Celsius
  static Temperature from_celsius(float celsius);
};

/**
 * @brief Direction and node ID pair (2 bytes)
 */
struct __attribute__((packed)) DirectionNode {
  Direction direction;  ///< Direction indicator
  NodeId node_id;       ///< Node identifier
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
};

/**
 * @brief Common message termination (CRC + prologue)
 * Size: 2 bytes
 */
struct __attribute__((packed)) MessageFrameEnd {
  uint8_t crc;                 ///< Checksum (CRC)
  ProtocolMarker prologue;     ///< Must be 0x55
};

/**
 * @brief Template function for enum-to-string conversion
 */
template<typename EnumType>
const char* enum_to_string(EnumType value, const std::map<EnumType, const char*>& mapping) {
  auto it = mapping.find(value);
  if (it != mapping.end()) {
    return it->second;
  }
  return "UNKNOWN";
}

// Static assertions
static_assert(sizeof(ProtocolMarker) == 1, "ProtocolMarker must be 1 byte");
static_assert(sizeof(MessageFrameEnd) == 2, "MessageFrameEnd must be 2 bytes (CRC + prologue)");

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
