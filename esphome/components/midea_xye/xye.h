#pragma once

#ifdef USE_ARDUINO

#include <cstdint>
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace ac {

// Protocol constants
constexpr uint8_t PROTOCOL_PREAMBLE = 0xAA;  ///< Start byte for all messages
constexpr uint8_t PROTOCOL_PROLOGUE = 0x55;  ///< End byte for all messages

constexpr uint8_t FROM_CLIENT = 0x00;  ///< Direction indicator: message from client (thermostat)
constexpr uint8_t TO_CLIENT = 0x00;    ///< Direction indicator: message to client (thermostat)

// Message lengths
constexpr uint8_t TX_MESSAGE_LENGTH = 16;  ///< Length of transmitted messages
constexpr uint8_t RX_MESSAGE_LENGTH = 32;  ///< Length of received messages

// TODO: Don't hardcode these IDs
constexpr uint8_t SERVER_ID = 0;  ///< ID of the HVAC unit (server)
constexpr uint8_t CLIENT_ID = 0;  ///< ID of the thermostat (client)

/**
 * @brief Command types sent from client (thermostat) to server (HVAC unit)
 */
enum class ClientCommand : uint8_t {
  QUERY = 0xC0,           ///< Query current status (basic)
  QUERY_EXTENDED = 0xC4,  ///< Query extended status (includes outdoor temp, static pressure)
  SET = 0xC3,             ///< Set operation parameters (mode, temp, fan, etc.)
  FOLLOW_ME = 0xC6,       ///< Follow-Me temperature update or static pressure setting
  LOCK = 0xCC,            ///< Lock the physical controls
  UNLOCK = 0xCD,          ///< Unlock the physical controls
};

/**
 * @brief Response command types from server (HVAC unit) to client (thermostat)
 */
enum class ServerCommand : uint8_t {
  QUERY_RESPONSE = 0xC0,  ///< Response to query command
  SET_RESPONSE = 0xC3,    ///< Response to set command
  LOCK_RESPONSE = 0xCC,   ///< Response to lock command
  UNLOCK_RESPONSE = 0xCD  ///< Response to unlock command
};

/**
 * @brief Operation modes for the HVAC unit
 */
enum class OperationMode : uint8_t {
  OFF = 0x00,   ///< Unit is off
  AUTO = 0x80,  ///< Automatic mode (heat/cool as needed)
  FAN = 0x81,   ///< Fan only mode
  DRY = 0x82,   ///< Dehumidify mode
  HEAT = 0x84,  ///< Heating mode
  COOL = 0x88   ///< Cooling mode
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
namespace ModeFlags {
constexpr uint8_t NORMAL = 0x00;      ///< Normal operation
constexpr uint8_t ECO = 0x01;         ///< Energy saving mode
constexpr uint8_t AUX_HEAT = 0x02;    ///< Auxiliary heat / Turbo mode
constexpr uint8_t SWING = 0x04;       ///< Swing mode enabled
constexpr uint8_t VENTILATION = 0x88; ///< Ventilation mode
}  // namespace ModeFlags

/**
 * @brief Operation flags from unit status
 */
namespace OperationFlags {
constexpr uint8_t WATER_PUMP = 0x04;  ///< Water pump active
constexpr uint8_t WATER_LOCK = 0x80;  ///< Water lock protection active
}  // namespace OperationFlags

/**
 * @brief Unit capabilities flags
 */
namespace Capabilities {
constexpr uint8_t EXTERNAL_TEMP = 0x80;  ///< Supports external temperature sensor
constexpr uint8_t SWING = 0x10;          ///< Supports swing function
}  // namespace Capabilities

/**
 * @brief Auto mode flag in operation mode byte
 */
constexpr uint8_t OP_MODE_AUTO_FLAG = 0x10;

/**
 * @brief Timer duration flags (combinable)
 */
namespace TimerFlags {
constexpr uint8_t TIMER_15MIN = 0x01;   ///< 15 minute increment
constexpr uint8_t TIMER_30MIN = 0x02;   ///< 30 minute increment
constexpr uint8_t TIMER_1HOUR = 0x04;   ///< 1 hour increment
constexpr uint8_t TIMER_2HOUR = 0x08;   ///< 2 hour increment
constexpr uint8_t TIMER_4HOUR = 0x10;   ///< 4 hour increment
constexpr uint8_t TIMER_8HOUR = 0x20;   ///< 8 hour increment
constexpr uint8_t TIMER_16HOUR = 0x40;  ///< 16 hour increment
constexpr uint8_t INVALID = 0x80;       ///< Timer not set/invalid
}  // namespace TimerFlags

/**
 * @brief Follow-Me subcommand types (used in TXData[10])
 */
enum class FollowMeSubcommand : uint8_t {
  UPDATE = 0x02,          ///< Regular temperature update
  STATIC_PRESSURE = 0x04, ///< Static pressure setting
  INIT = 0x06             ///< Initialization after mode change
};

/**
 * @brief Special temperature value for fan mode
 */
constexpr uint8_t TEMP_FAN_MODE = 0xFF;

/**
 * @brief Transmit message structure (Client to Server)
 * Total size: 16 bytes
 */
struct __attribute__((packed)) TransmitMessage {
  uint8_t preamble;            ///< [0] Must be 0xAA
  uint8_t command;             ///< [1] Command type (ClientCommand)
  uint8_t server_id;           ///< [2] Server (HVAC) ID
  uint8_t client_id1;          ///< [3] Client (thermostat) ID
  uint8_t direction;           ///< [4] Direction: FROM_CLIENT (0x00)
  uint8_t client_id2;          ///< [5] Client ID (repeated)
  uint8_t operation_mode;      ///< [6] Operation mode (OperationMode) for SET command
  uint8_t fan_mode;            ///< [7] Fan speed (FanMode) for SET command
  uint8_t target_temperature;  ///< [8] Target temperature or special value for SET/FOLLOW_ME
  uint8_t timer_start;         ///< [9] Start timer flags (TimerFlags) for SET command
  uint8_t timer_stop;          ///< [10] Stop timer flags (TimerFlags) for SET, or FollowMeSubcommand
  uint8_t mode_flags;          ///< [11] Mode flags (ModeFlags) for SET, or temperature for FOLLOW_ME
  uint8_t reserved1;           ///< [12] Reserved/unused
  uint8_t complement;          ///< [13] Bitwise complement of command byte (0xFF - command)
  uint8_t crc;                 ///< [14] Checksum (CRC)
  uint8_t prologue;            ///< [15] Must be 0x55
};

/**
 * @brief Common header fields for all receive messages
 */
struct __attribute__((packed)) ReceiveMessageHeader {
  uint8_t preamble;     ///< [0] Must be 0xAA
  uint8_t command;      ///< [1] Response command type (ServerCommand)
  uint8_t direction;    ///< [2] Direction: TO_CLIENT (0x00)
  uint8_t destination1; ///< [3] Destination client ID
  uint8_t source;       ///< [4] Source server ID
  uint8_t destination2; ///< [5] Destination client ID (repeated)
};

/**
 * @brief Query response message structure (Server to Client, command 0xC0)
 * Contains current unit status and sensor readings
 * Total size: 32 bytes
 */
struct __attribute__((packed)) QueryResponseMessage {
  ReceiveMessageHeader header;    ///< [0-5] Common header
  uint8_t unknown1;                ///< [6] Unknown/reserved
  uint8_t capabilities;            ///< [7] Unit capabilities flags (Capabilities)
  uint8_t operation_mode;          ///< [8] Current operation mode (OperationMode)
  uint8_t fan_mode;                ///< [9] Current fan mode (FanMode)
  uint8_t target_temperature;      ///< [10] Target temperature setpoint
  uint8_t t1_temperature;          ///< [11] Internal temperature sensor (T1)
  uint8_t t2a_temperature;         ///< [12] Temperature sensor 2A
  uint8_t t2b_temperature;         ///< [13] Temperature sensor 2B
  uint8_t t3_temperature;          ///< [14] Temperature sensor 3
  uint8_t current;                 ///< [15] Current draw (units TBD)
  uint8_t unknown2;                ///< [16] Unknown/reserved
  uint8_t timer_start;             ///< [17] Start timer setting (TimerFlags)
  uint8_t timer_stop;              ///< [18] Stop timer setting (TimerFlags)
  uint8_t unknown3;                ///< [19] Unknown/reserved
  uint8_t mode_flags;              ///< [20] Mode flags (ModeFlags)
  uint8_t operation_flags;         ///< [21] Operation status flags (OperationFlags)
  uint8_t error_flags_low;         ///< [22] Error flags byte 1
  uint8_t error_flags_high;        ///< [23] Error flags byte 2
  uint8_t protect_flags_low;       ///< [24] Protection flags byte 1
  uint8_t protect_flags_high;      ///< [25] Protection flags byte 2
  uint8_t ccm_communication_error_flags; ///< [26] CCM communication error flags
  uint8_t unknown4;                ///< [27] Unknown/reserved
  uint8_t unknown5;                ///< [28] Unknown/reserved
  uint8_t unknown6;                ///< [29] Unknown/reserved
  uint8_t crc;                     ///< [30] Checksum (CRC)
  uint8_t prologue;                ///< [31] Must be 0x55
};

/**
 * @brief Extended query response message structure (Server to Client, command 0xC4)
 * Contains outdoor temperature and static pressure information
 * Total size: 32 bytes
 */
struct __attribute__((packed)) ExtendedQueryResponseMessage {
  ReceiveMessageHeader header;  ///< [0-5] Common header
  uint8_t unknown1;             ///< [6] Unknown/reserved
  uint8_t unknown2;             ///< [7] Unknown/reserved
  uint8_t unknown3;             ///< [8] Unknown/reserved
  uint8_t unknown4;             ///< [9] Unknown/reserved
  uint8_t unknown5;             ///< [10] Unknown/reserved
  uint8_t unknown6;             ///< [11] Unknown/reserved
  uint8_t unknown7;             ///< [12] Unknown/reserved
  uint8_t unknown8;             ///< [13] Unknown/reserved
  uint8_t unknown9;             ///< [14] Unknown/reserved
  uint8_t unknown10;            ///< [15] Unknown/reserved
  uint8_t unknown11;            ///< [16] Unknown/reserved
  uint8_t unknown12;            ///< [17] Unknown/reserved
  uint8_t target_temperature;   ///< [18] Target temperature (may be in Fahrenheit with offset)
  uint8_t unknown13;            ///< [19] Unknown/reserved
  uint8_t unknown14;            ///< [20] Unknown/reserved
  uint8_t outdoor_temperature;  ///< [21] Outdoor temperature sensor reading
  uint8_t unknown15;            ///< [22] Unknown/reserved
  uint8_t unknown16;            ///< [23] Unknown/reserved
  uint8_t static_pressure;      ///< [24] Static pressure setting (lower 4 bits)
  uint8_t unknown17;            ///< [25] Unknown/reserved
  uint8_t unknown18;            ///< [26] Unknown/reserved
  uint8_t unknown19;            ///< [27] Unknown/reserved
  uint8_t unknown20;            ///< [28] Unknown/reserved
  uint8_t unknown21;            ///< [29] Unknown/reserved
  uint8_t crc;                  ///< [30] Checksum (CRC)
  uint8_t prologue;             ///< [31] Must be 0x55
};

/**
 * @brief Generic receive message that can be any response type
 */
struct __attribute__((packed)) ReceiveMessage {
  ReceiveMessageHeader header;  ///< [0-5] Common header
  uint8_t data[24];             ///< [6-29] Variable data depending on command type
  uint8_t crc;                  ///< [30] Checksum (CRC)
  uint8_t prologue;             ///< [31] Must be 0x55
};

/**
 * @brief Union for transmit data - allows access as both byte array and struct
 */
union TransmitData {
  uint8_t raw[TX_MESSAGE_LENGTH];  ///< Raw byte array for UART transmission
  TransmitMessage message;         ///< Structured access to message fields

  /**
   * @brief Pretty print the transmit message for debugging
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const {
    ESP_LOGD(tag, "TX Message:");
    ESP_LOGD(tag, "  Command: 0x%02X", message.command);
    ESP_LOGD(tag, "  Operation Mode: 0x%02X", message.operation_mode);
    ESP_LOGD(tag, "  Fan Mode: 0x%02X", message.fan_mode);
    ESP_LOGD(tag, "  Target Temp: %d", message.target_temperature);
    ESP_LOGD(tag, "  Timer Start: 0x%02X", message.timer_start);
    ESP_LOGD(tag, "  Timer Stop/Subcmd: 0x%02X", message.timer_stop);
    ESP_LOGD(tag, "  Mode Flags/Data: 0x%02X", message.mode_flags);
    ESP_LOGD(tag, "  Raw: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
             raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
             raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
  }
};

/**
 * @brief Union for receive data - allows access as both byte array and structs
 */
union ReceiveData {
  uint8_t raw[RX_MESSAGE_LENGTH];                      ///< Raw byte array from UART
  ReceiveMessage message;                              ///< Generic structured access
  QueryResponseMessage query_response;                 ///< Query response (0xC0) access
  ExtendedQueryResponseMessage extended_query_response; ///< Extended query response (0xC4) access

  /**
   * @brief Pretty print the receive message for debugging
   * @param tag Log tag to use
   */
  void print_debug(const char *tag) const {
    ESP_LOGD(tag, "RX Message:");
    ESP_LOGD(tag, "  Command: 0x%02X", message.header.command);
    
    if (message.header.command == static_cast<uint8_t>(ServerCommand::QUERY_RESPONSE)) {
      ESP_LOGD(tag, "  Query Response:");
      ESP_LOGD(tag, "    Operation Mode: 0x%02X", query_response.operation_mode);
      ESP_LOGD(tag, "    Fan Mode: 0x%02X", query_response.fan_mode);
      ESP_LOGD(tag, "    Target Temp: %d", query_response.target_temperature);
      ESP_LOGD(tag, "    T1 Temp: 0x%02X", query_response.t1_temperature);
      ESP_LOGD(tag, "    T2A Temp: 0x%02X", query_response.t2a_temperature);
      ESP_LOGD(tag, "    T2B Temp: 0x%02X", query_response.t2b_temperature);
      ESP_LOGD(tag, "    T3 Temp: 0x%02X", query_response.t3_temperature);
      ESP_LOGD(tag, "    Current: %d", query_response.current);
      ESP_LOGD(tag, "    Mode Flags: 0x%02X", query_response.mode_flags);
      ESP_LOGD(tag, "    Error Flags: 0x%04X", 
               (query_response.error_flags_high << 8) | query_response.error_flags_low);
      ESP_LOGD(tag, "    Protect Flags: 0x%04X", 
               (query_response.protect_flags_high << 8) | query_response.protect_flags_low);
    }
    
    ESP_LOGD(tag, "  Raw: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:"
                  "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
             raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
             raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15],
             raw[16], raw[17], raw[18], raw[19], raw[20], raw[21], raw[22], raw[23],
             raw[24], raw[25], raw[26], raw[27], raw[28], raw[29], raw[30], raw[31]);
  }
};

// Static assertions to ensure struct sizes are correct
static_assert(sizeof(TransmitMessage) == TX_MESSAGE_LENGTH, "TransmitMessage size must be 16 bytes");
static_assert(sizeof(QueryResponseMessage) == RX_MESSAGE_LENGTH, "QueryResponseMessage size must be 32 bytes");
static_assert(sizeof(ExtendedQueryResponseMessage) == RX_MESSAGE_LENGTH, "ExtendedQueryResponseMessage size must be 32 bytes");
static_assert(sizeof(ReceiveMessage) == RX_MESSAGE_LENGTH, "ReceiveMessage size must be 32 bytes");
static_assert(sizeof(TransmitData) == TX_MESSAGE_LENGTH, "TransmitData size must be 16 bytes");
static_assert(sizeof(ReceiveData) == RX_MESSAGE_LENGTH, "ReceiveData size must be 32 bytes");

}  // namespace ac
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
