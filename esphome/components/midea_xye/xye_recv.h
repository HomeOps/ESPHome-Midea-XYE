#pragma once

#ifdef USE_ARDUINO

#include "xye.h"

namespace esphome {
namespace midea {
namespace xye {

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
 * 
 * Field mappings verified against wtahler's implementation:
 * - Byte indices in comments are absolute positions in 32-byte message
 * - Struct offsets are relative to data section (subtract 6 for frame header)
 */
struct __attribute__((packed)) QueryResponseData {
  uint8_t unknown1;                ///< [6] Unknown/reserved
  Capabilities capabilities;       ///< [7] Unit capabilities flags
  OperationMode operation_mode;    ///< [8] Current operation mode
  FanMode fan_mode;                ///< [9] Current fan mode
  Temperature target_temperature;  ///< [10] Target temperature setpoint
  Temperature t1_temperature;      ///< [11] Internal/inlet air temperature sensor (T1) - room temperature
  Temperature t2a_temperature;     ///< [12] Indoor coil inlet temperature (T2A) - refrigerant entering
  Temperature t2b_temperature;     ///< [13] Indoor coil outlet temperature (T2B) - refrigerant leaving
  Temperature t3_temperature;      ///< [14] Outdoor coil/ambient temperature (T3)
  uint8_t current;                 ///< [15] Current draw (units TBD, often reads 0xFF)
  uint8_t unknown2;                ///< [16] Unknown/reserved
  uint8_t timer_start;             ///< [17] Start timer setting (combinable TimerFlags)
  uint8_t timer_stop;              ///< [18] Stop timer setting (combinable TimerFlags)
  uint8_t unknown3;                ///< [19] Unknown/reserved
  ModeFlags mode_flags;            ///< [20] Mode flags (ECO, AUX_HEAT, SWING, etc.)
  OperationFlags operation_flags;  ///< [21] Operation status flags (water pump, water lock)
  Flags16 error_flags;             ///< [22-23] Error flags (16-bit) - E1/E2 error codes
  Flags16 protect_flags;           ///< [24-25] Protection flags (16-bit)
  CcmErrorFlags ccm_communication_error_flags; ///< [26] CCM communication error flags
  uint8_t unknown4;                ///< [27] Unknown/reserved
  uint8_t unknown5;                ///< [28] Unknown/reserved
  uint8_t unknown6;                ///< [29] Unknown/reserved

  /**
   * @brief Print debug information for query response data
   * @param tag Log tag to use
   * @param left Bytes remaining to read
   * @param level Log level (ESPHOME_LOG_LEVEL_DEBUG, ESPHOME_LOG_LEVEL_INFO, ESPHOME_LOG_LEVEL_ERROR, etc.)
   * @return Updated bytes remaining
   */
  size_t print_debug(const char *tag, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

/**
 * @brief Extended query response data (Server to Client, command 0xC4)
 * Contains outdoor temperature, static pressure, and engineering/diagnostic information
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * This frame provides engineering and diagnostic data that complements the basic C0 query:
 * - Outdoor temperature and static pressure
 * - Compressor and fan status flags
 * - System configuration and protection states
 * - Indoor unit addressing and ESP profile settings
 */
struct __attribute__((packed)) ExtendedQueryResponseData {
  FanPwm indoor_fan_pwm;       ///< [0] Indoor fan PWM control (0x00 = not exposed on some models)
  FanTach indoor_fan_tach;      ///< [1] Indoor fan tachometer feedback (0x00 = not exposed on some models)
  CompressorFlags compressor_flags;     ///< [2] Compressor status flags (bit 7: compressor active/running)
  EspProfile esp_profile;          ///< [3] Airflow/ESP (External Static Pressure) profile (0x30 = medium ESP)
  ProtectionFlags protection_flags;     ///< [4] Protection and outdoor fan state flags (bits: defrost, anti-freeze, outdoor fan)
  Temperature coil_inlet_temp;      ///< [5] Coil inlet temperature (0x00 = unused on some models)
  Temperature coil_outlet_temp;     ///< [6] Coil outlet temperature (0x00 = unused on some models)
  Temperature discharge_temp;       ///< [7] Discharge temperature (0x00 = unused on some models)
  ValvePosition expansion_valve_pos;  ///< [8] Expansion valve position (0x00 = unused on some models)
  uint8_t reserved1;            ///< [9] Reserved/unused
  SystemStatusFlags system_status_flags;  ///< [10] System status flags (bit 7: enabled, bit 2: wired controller present)
  NodeId indoor_unit_address;  ///< [11] Indoor unit address/zone index (0x01 = address 1)
  Temperature target_temperature;   ///< [12] Target temperature (may be in Fahrenheit with offset)
  Flags16BigEndian compressor_freq_or_fan_rpm; ///< [13-14] 16-bit engineering value (compressor Hz or outdoor fan RPM), big-endian: high byte at [13], low byte at [14]
  Temperature outdoor_temperature;  ///< [15] Outdoor temperature sensor reading
  uint8_t reserved2;            ///< [16] Reserved/unused
  uint8_t reserved3;            ///< [17] Reserved/unused
  uint8_t static_pressure;      ///< [18] Static pressure setting (lower 4 bits)
  uint8_t reserved4;            ///< [19] Reserved/unused
  SubsystemFlags subsystem_ok_compressor;    ///< [20] Compressor subsystem OK flag (0x80 = OK, other bits = protections)
  SubsystemFlags subsystem_ok_outdoor_fan;   ///< [21] Outdoor fan subsystem OK flag (0x80 = OK, other bits = protections)
  SubsystemFlags subsystem_ok_4way_valve;    ///< [22] 4-way valve subsystem OK flag (0x80 = OK, other bits = protections)
  SubsystemFlags subsystem_ok_inverter;      ///< [23] Inverter module subsystem OK flag (0x80 = OK, other bits = protections)

  /**
   * @brief Print debug information for extended query response data
   * @param tag Log tag to use
   * @param left Bytes remaining to read
   * @param level Log level (ESPHOME_LOG_LEVEL_DEBUG, ESPHOME_LOG_LEVEL_INFO, ESPHOME_LOG_LEVEL_ERROR, etc.)
   * @return Updated bytes remaining
   */
  size_t print_debug(const char *tag, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

/**
 * @brief SET command response data (Server to Client, command 0xC3)
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * Uses QueryResponseData structure as it contains the most information.
 * This allows the response to be interpreted with meaningful field names.
 */
using SetResponseData = QueryResponseData;

/**
 * @brief FOLLOW_ME command response data (Server to Client, command 0xC6)
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * Uses QueryResponseData structure as it contains the most information.
 * This allows the response to be interpreted with meaningful field names.
 */
using FollowMeResponseData = QueryResponseData;

/**
 * @brief LOCK command response data (Server to Client, command 0xCC)
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * Uses QueryResponseData structure as it contains the most information.
 * This allows the response to be interpreted with meaningful field names.
 */
using LockResponseData = QueryResponseData;

/**
 * @brief UNLOCK command response data (Server to Client, command 0xCD)
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * Uses QueryResponseData structure as it contains the most information.
 * This allows the response to be interpreted with meaningful field names.
 */
using UnlockResponseData = QueryResponseData;

/**
 * @brief Generic receive message data - typedef to QueryResponseData
 * Size: 24 bytes (bytes 6-29, excluding frame, CRC, and prologue)
 * 
 * Fallback for unknown or unspecified command types.
 * Uses QueryResponseData as it contains the most information.
 */
using ReceiveMessageData = QueryResponseData;

/**
 * @brief Union for receive message data payloads
 * Provides type-safe access to different data types based on command
 */
union ReceiveMessageDataUnion {
  ReceiveMessageData generic;                       ///< Generic data access (fallback)
  QueryResponseData query_response;                 ///< Query response (0xC0) data
  ExtendedQueryResponseData extended_query_response;///< Extended query response (0xC4) data
  SetResponseData set_response;                     ///< Set response (0xC3) data
  FollowMeResponseData follow_me_response;          ///< Follow-Me response (0xC6) data
  LockResponseData lock_response;                   ///< Lock response (0xCC) data
  UnlockResponseData unlock_response;               ///< Unlock response (0xCD) data
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
   * @param left Bytes remaining (received size)
   * @param tag Log tag to use
   * @param level Log level (ESPHOME_LOG_LEVEL_DEBUG, ESPHOME_LOG_LEVEL_INFO, ESPHOME_LOG_LEVEL_ERROR, etc.)
   * @return Updated bytes remaining
   */
  size_t print_debug(size_t left, const char *tag, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

// Static assertions to ensure struct sizes are correct
static_assert(sizeof(ReceiveMessageHeader) == 5, "ReceiveMessageHeader must be 5 bytes");
static_assert(sizeof(ReceiveMessageFrame) == sizeof(ProtocolMarker) + sizeof(ReceiveMessageHeader), "ReceiveMessageFrame must be preamble + header");
static_assert(sizeof(QueryResponseData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "QueryResponseData size must exclude frame and frame_end");
static_assert(sizeof(ExtendedQueryResponseData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ExtendedQueryResponseData size must exclude frame and frame_end");
static_assert(sizeof(ReceiveMessageData) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ReceiveMessageData size must exclude frame and frame_end");
static_assert(sizeof(ReceiveMessageDataUnion) == RX_MESSAGE_LENGTH - sizeof(ReceiveMessageFrame) - sizeof(MessageFrameEnd), "ReceiveMessageDataUnion size must be 24 bytes");
static_assert(sizeof(ReceiveData) == RX_MESSAGE_LENGTH, "ReceiveData size must match RX_MESSAGE_LENGTH");

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
