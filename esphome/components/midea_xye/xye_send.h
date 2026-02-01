#pragma once

#ifdef USE_ARDUINO

#include "xye.h"

namespace esphome {
namespace midea {
namespace xye {

/**
 * @brief Transmit message header (without preamble)
 * Size: 5 bytes (bytes 1-5)
 */
struct __attribute__((packed)) TransmitMessageHeader {
  Command command;                   ///< [0] Command type
  NodeId server_id;                  ///< [1] Destination/Server ID (0x00..0x3F for specific unit, 0xFF for broadcast)
  NodeId client_id1;                 ///< [2] Source/Master ID (0x00..0x3F)
  DirectionNode direction_node;      ///< [3-4] Direction (0x80 from master) and source ID
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
   * @param command The command type (for context, all fields are always printed)
   * @param left Bytes remaining to read
   * @param level Log level (ESPHOME_LOG_LEVEL_DEBUG, ESPHOME_LOG_LEVEL_INFO, ESPHOME_LOG_LEVEL_ERROR, etc.)
   * @return Updated bytes remaining
   */
  size_t print_debug(const char *tag, Command command, size_t left, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
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
   * @param left Bytes remaining (for TX always TX_MESSAGE_LENGTH)
   * @param tag Log tag to use
   * @param level Log level (ESPHOME_LOG_LEVEL_DEBUG, ESPHOME_LOG_LEVEL_INFO, ESPHOME_LOG_LEVEL_ERROR, etc.)
   * @return Updated bytes remaining
   */
  size_t print_debug(size_t left, const char *tag, int level = ESPHOME_LOG_LEVEL_DEBUG) const;
};

// Static assertions to ensure struct sizes are correct
static_assert(sizeof(TransmitMessageHeader) == 5, "TransmitMessageHeader must be 5 bytes");
static_assert(sizeof(TransmitMessageFrame) == sizeof(ProtocolMarker) + sizeof(TransmitMessageHeader), "TransmitMessageFrame must be preamble + header");
static_assert(sizeof(TransmitMessageData) == TX_MESSAGE_LENGTH - sizeof(TransmitMessageFrame) - sizeof(MessageFrameEnd), "TransmitMessageData size must exclude frame and frame_end");
static_assert(sizeof(TransmitData) == TX_MESSAGE_LENGTH, "TransmitData size must match TX_MESSAGE_LENGTH");

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
