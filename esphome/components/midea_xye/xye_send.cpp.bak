#ifdef USE_ARDUINO

#include "xye_send.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// TransmitMessageData methods
void TransmitMessageData::print_debug(const char *tag, Command command, int level) const {
  (void)command;  // Unused parameter, kept for API consistency
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  TransmitMessageData:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    operation_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    fan_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode));
  target_temperature.print_debug(tag, "target_temperature", level);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_start: 0x%02X"), timer_start);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_stop: 0x%02X"), timer_stop);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    mode_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved1: 0x%02X"), reserved1);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    complement: 0x%02X"), complement);
}

// TransmitData methods
void TransmitData::print_debug(const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("TX Message:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame Header:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    preamble: 0x%02X"), static_cast<uint8_t>(message.frame.preamble));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    command: 0x%02X (%s)"), 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    server_id: 0x%02X"), message.frame.header.server_id);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    client_id1: 0x%02X"), message.frame.header.client_id1);
  message.frame.header.direction_node.print_debug(tag, "direction_node", level);
  
  // Delegate to the data struct's print_debug method
  message.data.standard.print_debug(tag, message.frame.header.command, level);
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame End:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    crc: 0x%02X"), message.frame_end.crc);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    prologue: 0x%02X"), static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
