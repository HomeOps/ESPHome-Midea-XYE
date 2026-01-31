#ifdef USE_ARDUINO

#include "xye_send.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// TransmitMessageData methods
void TransmitMessageData::print_debug(const char *tag, Command command, int level) const {
  (void)command;  // Unused parameter, kept for API consistency
  XYE_LOG_LEVEL(level, tag, "  TransmitMessageData:");
  XYE_LOG_LEVEL(level, tag, "    operation_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode));
  XYE_LOG_LEVEL(level, tag, "    fan_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode));
  XYE_LOG_LEVEL(level, tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  XYE_LOG_LEVEL(level, tag, "    timer_start: 0x%02X", timer_start);
  XYE_LOG_LEVEL(level, tag, "    timer_stop: 0x%02X", timer_stop);
  XYE_LOG_LEVEL(level, tag, "    mode_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags));
  XYE_LOG_LEVEL(level, tag, "    reserved1: 0x%02X", reserved1);
  XYE_LOG_LEVEL(level, tag, "    complement: 0x%02X", complement);
}

// TransmitData methods
void TransmitData::print_debug(const char *tag, int level) const {
  XYE_LOG_LEVEL(level, tag, "TX Message:");
  XYE_LOG_LEVEL(level, tag, "  Frame Header:");
  XYE_LOG_LEVEL(level, tag, "    preamble: 0x%02X", static_cast<uint8_t>(message.frame.preamble));
  XYE_LOG_LEVEL(level, tag, "    command: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command));
  XYE_LOG_LEVEL(level, tag, "    server_id: 0x%02X", message.frame.header.server_id);
  XYE_LOG_LEVEL(level, tag, "    client_id1: 0x%02X", message.frame.header.client_id1);
  XYE_LOG_LEVEL(level, tag, "    direction: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.direction_node.direction),
           enum_to_string(message.frame.header.direction_node.direction));
  XYE_LOG_LEVEL(level, tag, "    node_id: 0x%02X", message.frame.header.direction_node.node_id);
  
  // Delegate to the data struct's print_debug method
  message.data.standard.print_debug(tag, message.frame.header.command, level);
  
  XYE_LOG_LEVEL(level, tag, "  Frame End:");
  XYE_LOG_LEVEL(level, tag, "    crc: 0x%02X", message.frame_end.crc);
  XYE_LOG_LEVEL(level, tag, "    prologue: 0x%02X", static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
