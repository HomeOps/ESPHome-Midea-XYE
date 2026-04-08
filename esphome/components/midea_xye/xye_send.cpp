#ifdef USE_ARDUINO

#include "xye_send.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// TransmitMessageData methods
size_t TransmitMessageData::print_debug(const char *tag, Command command, size_t left, int level) const {
  (void)command;  // Unused parameter, kept for API consistency
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  TransmitMessageData:"));
  
  left = print_debug_enum(tag, "operation_mode", operation_mode, left, level);
  left = print_debug_enum(tag, "fan_mode", fan_mode, left, level);
  left = target_temperature.print_debug(tag, "target_temperature", left, level);
  left = print_debug_uint8(tag, "timer_start", timer_start, left, level);
  left = print_debug_uint8(tag, "timer_stop", timer_stop, left, level);
  left = print_debug_enum(tag, "mode_flags", mode_flags, left, level);
  left = print_debug_uint8(tag, "reserved1", reserved1, left, level);
  left = print_debug_uint8(tag, "complement", complement, left, level);
  
  return left;
}

// TransmitData methods
size_t TransmitData::print_debug(const char *tag, size_t left, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("TX Message:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame Header:"));
  
  left = print_debug_uint8(tag, "preamble", static_cast<uint8_t>(message.frame.preamble), left, level);
  left = print_debug_enum(tag, "command", message.frame.header.command, left, level);
  left = print_debug_uint8(tag, "server_id", message.frame.header.server_id, left, level);
  left = print_debug_uint8(tag, "client_id1", message.frame.header.client_id1, left, level);
  left = message.frame.header.direction_node.print_debug(tag, "direction_node", left, level);
  
  // Delegate to the data struct's print_debug method
  left = message.data.standard.print_debug(tag, message.frame.header.command, left, level);
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame End:"));
  left = print_debug_uint8(tag, "crc", message.frame_end.crc, left, level);
  left = print_debug_uint8(tag, "prologue", static_cast<uint8_t>(message.frame_end.prologue), left, level);
  
  return left;
}

TransmitData::TransmitData(Command cmd) noexcept {
  message.frame.preamble = ProtocolMarker::PREAMBLE;
  message.frame.header.command = cmd;
  message.frame.header.server_id = SERVER_ID;
  message.frame.header.client_id1 = CLIENT_ID;
  message.frame.header.direction_node.direction = Direction::FROM_CLIENT;
  message.frame.header.direction_node.node_id = CLIENT_ID;
  message.data.standard.operation_mode = OperationMode::OFF;
  message.data.standard.fan_mode = FanMode::FAN_OFF;
  message.data.standard.target_temperature.value = 0;
  message.data.standard.timer_start = 0;
  message.data.standard.timer_stop = 0;
  message.data.standard.mode_flags = ModeFlags::NORMAL;
  message.data.standard.reserved1 = 0;
  message.data.standard.complement = static_cast<uint8_t>(0xFF - static_cast<uint8_t>(cmd));
  message.frame_end.crc = 0;
  message.frame_end.prologue = ProtocolMarker::PROLOGUE;
}

void TransmitData::update_crc() noexcept {
  message.frame_end.crc = compute_protocol_crc(raw, TX_MESSAGE_LENGTH);
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
