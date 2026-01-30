#pragma once

#ifdef USE_ARDUINO

#include <cstdint>
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
 * @brief Transmit message header (without preamble)
 * Size: 5 bytes (bytes 1-5)
 */
struct __attribute__((packed)) TransmitMessageHeader {
  Command command;                   ///< [0] Command type
  NodeId server_id;                  ///< [1] Server (HVAC) ID
  NodeId client_id1;                 ///< [2] Client (thermostat) ID
  DirectionNode direction_node;      ///< [3-4] Direction and client ID
};

/**
 * @brief Common frame for transmit messages (preamble + header)
 * Size: 6 bytes (preamble + 5-byte header)
 */
struct __attribute__((packed)) TransmitMessageFrame {
  ProtocolMarker preamble;           ///< [0] Must be 0xAA
  TransmitMessageHeader header;      ///< [1-5] Common header for all transmit messages
};

/**
 * @brief Transmit message data (without frame, CRC, and prologue)
 * Size: 8 bytes (bytes 6-13)
 */
struct __attribute__((packed)) TransmitMessageData {
  OperationMode operation_mode;      ///< [0] Operation mode for SET command
  FanMode fan_mode;                  ///< [1] Fan speed for SET command
  Temperature target_temperature;    ///< [2] Target temperature for SET/FOLLOW_ME
  uint8_t timer_start;               ///< [3] Start timer flags for SET command (combinable TimerFlags)
  uint8_t timer_stop;                ///< [4] Stop timer flags for SET (combinable TimerFlags), or FollowMeSubcommand
  ModeFlags mode_flags;              ///< [5] Mode flags for SET, or temperature for FOLLOW_ME
  uint8_t reserved1;                 ///< [6] Reserved/unused
  uint8_t complement;                ///< [7] Bitwise complement of command byte (0xFF - command)

  /**
   * @brief Print debug information for this data struct
   * @param tag Log tag to use
   * @param command The command type to determine which fields are relevant
   */
  void print_debug(const char *tag, Command command) const;
};

/**
 * @brief Common header fields for all receive messages (without preamble)
 */
struct __attribute__((packed)) ReceiveMessageHeader {
  Command command;      ///< [0] Response command type
  Direction direction;  ///< [1] Direction: TO_CLIENT
  NodeId destination1;  ///< [2] Destination client ID
  NodeId source;        ///< [3] Source server ID
  NodeId destination2;  ///< [4] Destination client ID (repeated)
};

/**
 * @brief Common frame for all receive messages (preamble + header)
 * Size: 6 bytes (preamble + 5-byte header)
 */
struct __attribute__((packed)) ReceiveMessageFrame {
  ProtocolMarker preamble;      ///< [0] Must be 0xAA
  ReceiveMessageHeader header;  ///< [1-5] Common header for all receive messages
};

/**
 * @brief Query response data (Server to Client, command 0xC0)
 * Contains current unit status and sensor readings
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 */
struct __attribute__((packed)) QueryResponseData {
  uint8_t unknown1;                ///< [0] Unknown/reserved
  Capabilities capabilities;       ///< [1] Unit capabilities flags
  OperationMode operation_mode;    ///< [2] Current operation mode
  FanMode fan_mode;                ///< [3] Current fan mode
  Temperature target_temperature;  ///< [4] Target temperature setpoint
  Temperature t1_temperature;      ///< [5] Internal temperature sensor (T1)
  Temperature t2a_temperature;     ///< [6] Temperature sensor 2A
  Temperature t2b_temperature;     ///< [7] Temperature sensor 2B
  Temperature t3_temperature;      ///< [8] Temperature sensor 3
  uint8_t current;                 ///< [9] Current draw (units TBD)
  uint8_t unknown2;                ///< [10] Unknown/reserved
  uint8_t timer_start;             ///< [11] Start timer setting (combinable TimerFlags)
  uint8_t timer_stop;              ///< [12] Stop timer setting (combinable TimerFlags)
  uint8_t unknown3;                ///< [13] Unknown/reserved
  ModeFlags mode_flags;            ///< [14] Mode flags
  OperationFlags operation_flags;  ///< [15] Operation status flags
  Flags16 error_flags;             ///< [16-17] Error flags (16-bit)
  Flags16 protect_flags;           ///< [18-19] Protection flags (16-bit)
  CcmErrorFlags ccm_communication_error_flags; ///< [20] CCM communication error flags
  uint8_t unknown4;                ///< [21] Unknown/reserved
  uint8_t unknown5;                ///< [22] Unknown/reserved
  uint8_t unknown6;                ///< [23] Unknown/reserved

  /**
   * @brief Print debug information for query response data
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const;
};

/**
 * @brief Extended query response data (Server to Client, command 0xC4)
 * Contains outdoor temperature and static pressure information
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 */
struct __attribute__((packed)) ExtendedQueryResponseData {
  uint8_t unknown1;             ///< [0] Unknown/reserved
  uint8_t unknown2;             ///< [1] Unknown/reserved
  uint8_t unknown3;             ///< [2] Unknown/reserved
  uint8_t unknown4;             ///< [3] Unknown/reserved
  uint8_t unknown5;             ///< [4] Unknown/reserved
  uint8_t unknown6;             ///< [5] Unknown/reserved
  uint8_t unknown7;             ///< [6] Unknown/reserved
  uint8_t unknown8;             ///< [7] Unknown/reserved
  uint8_t unknown9;             ///< [8] Unknown/reserved
  uint8_t unknown10;            ///< [9] Unknown/reserved
  uint8_t unknown11;            ///< [10] Unknown/reserved
  uint8_t unknown12;            ///< [11] Unknown/reserved
  Temperature target_temperature;   ///< [12] Target temperature (may be in Fahrenheit with offset)
  uint8_t unknown13;            ///< [13] Unknown/reserved
  uint8_t unknown14;            ///< [14] Unknown/reserved
  Temperature outdoor_temperature;  ///< [15] Outdoor temperature sensor reading
  uint8_t unknown16;            ///< [16] Unknown/reserved
  uint8_t unknown17;            ///< [17] Unknown/reserved
  uint8_t static_pressure;      ///< [18] Static pressure setting (lower 4 bits)
  uint8_t unknown18;            ///< [19] Unknown/reserved
  uint8_t unknown19;            ///< [20] Unknown/reserved
  uint8_t unknown20;            ///< [21] Unknown/reserved
  uint8_t unknown21;            ///< [22] Unknown/reserved
  uint8_t unknown22;            ///< [23] Unknown/reserved

  /**
   * @brief Print debug information for extended query response data
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const;
};

/**
 * @brief Generic receive message data
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 */
struct __attribute__((packed)) ReceiveMessageData {
  uint8_t data[24];             ///< [0-23] Variable data depending on command type

  /**
   * @brief Print debug information for generic receive data
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const;
};

/**
 * @brief Union for transmit message data payloads
 * Provides type-safe access to different data types based on command
 */
union TransmitMessageDataUnion {
  TransmitMessageData standard;  ///< Standard transmit data (SET, QUERY, etc.)
};

static_assert(sizeof(TransmitMessageDataUnion) == TX_MESSAGE_LENGTH - sizeof(TransmitMessageFrame) - sizeof(MessageFrameEnd), 
              "TransmitMessageDataUnion size must match TX_DATA_LENGTH");

/**
 * @brief Union for transmit data - allows access as both byte array and struct
 */
union TransmitData {
  uint8_t raw[TX_MESSAGE_LENGTH];  ///< Raw byte array for UART transmission
  struct __attribute__((packed)) {
    TransmitMessageFrame frame;         ///< [0-5] Common frame (preamble + header)
    TransmitMessageDataUnion data;      ///< [6-13] Data union for different message types
    MessageFrameEnd frame_end;          ///< [14-15] CRC and prologue
  } message;

  /**
   * @brief Pretty print the transmit message for debugging
   * Takes into account the kind of message based on command type
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const;
};

/**
 * @brief Union for receive message data payloads
 * Provides type-safe access to different data types based on command
 */
union ReceiveMessageDataUnion {
  ReceiveMessageData generic;                       ///< Generic data access
  QueryResponseData query_response;                 ///< Query response (0xC0) data
  ExtendedQueryResponseData extended_query_response;///< Extended query response (0xC4) data
};

/**
 * @brief Union for receive data - allows access as both byte array and struct
 * Provides type-safe access to different message types based on command
 */
union ReceiveData {
  uint8_t raw[RX_MESSAGE_LENGTH];                   ///< Raw byte array from UART
  struct __attribute__((packed)) {
    ReceiveMessageFrame frame;                      ///< [0-5] Common frame (preamble + header)
    ReceiveMessageDataUnion data;                   ///< [6-29] Data union for different message types
    MessageFrameEnd frame_end;                      ///< [30-31] CRC and prologue
  } message;

  /**
   * @brief Get the command type from the message
   * @return The command type
   */
  Command get_command() const;

  /**
   * @brief Pretty print the receive message for debugging
   * Takes into account the kind of message based on command type
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const;
};

// Static assertions to ensure struct sizes are correct
static_assert(sizeof(ProtocolMarker) == 1, "ProtocolMarker must be 1 byte");
static_assert(sizeof(MessageFrameEnd) == 2, "MessageFrameEnd must be 2 bytes (CRC + prologue)");
static_assert(sizeof(TransmitMessageHeader) == 5, "TransmitMessageHeader must be 5 bytes");
static_assert(sizeof(TransmitMessageFrame) == sizeof(ProtocolMarker) + sizeof(TransmitMessageHeader), "TransmitMessageFrame must be preamble + header");
static_assert(sizeof(TransmitMessageData) == TX_MESSAGE_LENGTH - sizeof(TransmitMessageFrame) - sizeof(MessageFrameEnd), "TransmitMessageData size must exclude frame and frame_end");
static_assert(sizeof(ReceiveMessageHeader) == 5, "ReceiveMessageHeader must be 5 bytes");
static_assert(sizeof(ReceiveMessageFrame) == sizeof(ProtocolMarker) + sizeof(ReceiveMessageHeader), "ReceiveMessageFrame must be preamble + header");
static_assert(sizeof(QueryResponseData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "QueryResponseData size must exclude frame and frame_end");
static_assert(sizeof(ExtendedQueryResponseData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ExtendedQueryResponseData size must exclude frame and frame_end");
static_assert(sizeof(ReceiveMessageData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ReceiveMessageData size must exclude frame and frame_end");
static_assert(sizeof(ReceiveMessageDataUnion) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ReceiveMessageDataUnion size must be 24 bytes");
static_assert(sizeof(TransmitData) == TX_MESSAGE_LENGTH, "TransmitData size must match TX_MESSAGE_LENGTH");
static_assert(sizeof(ReceiveData) == RX_MESSAGE_LENGTH, "ReceiveData size must match RX_MESSAGE_LENGTH");

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
