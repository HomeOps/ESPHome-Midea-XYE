#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// QueryResponseData methods
size_t QueryResponseData::print_debug(const char *tag, size_t left, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  QueryResponseData:"));
  
  left = print_debug_uint8(tag, "unknown1", unknown1, left, level);
  left = print_debug_enum(tag, "capabilities", capabilities, left, level);
  left = print_debug_enum(tag, "operation_mode", operation_mode, left, level);
  left = print_debug_enum(tag, "fan_mode", fan_mode, left, level);
  left = target_temperature.print_debug(tag, "target_temperature", left, level);
  left = t1_temperature.print_debug(tag, "t1_temperature", left, level);
  left = t2a_temperature.print_debug(tag, "t2a_temperature", left, level);
  left = t2b_temperature.print_debug(tag, "t2b_temperature", left, level);
  left = t3_temperature.print_debug(tag, "t3_temperature", left, level);
  left = print_debug_uint8(tag, "current", current, left, level);
  left = print_debug_uint8(tag, "unknown2", unknown2, left, level);
  left = print_debug_uint8(tag, "timer_start", timer_start, left, level);
  left = print_debug_uint8(tag, "timer_stop", timer_stop, left, level);
  left = print_debug_uint8(tag, "unknown3", unknown3, left, level);
  left = print_debug_enum(tag, "mode_flags", mode_flags, left, level);
  left = print_debug_enum(tag, "operation_flags", operation_flags, left, level);
  left = error_flags.print_debug(tag, "error_flags", left, level);
  left = protect_flags.print_debug(tag, "protect_flags", left, level);
  left = print_debug_enum(tag, "ccm_communication_error_flags", ccm_communication_error_flags, left, level);
  left = print_debug_uint8(tag, "unknown4", unknown4, left, level);
  left = print_debug_uint8(tag, "unknown5", unknown5, left, level);
  left = print_debug_uint8(tag, "unknown6", unknown6, left, level);
  
  return left;
}

// ExtendedQueryResponseData methods
size_t ExtendedQueryResponseData::print_debug(const char *tag, size_t left, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ExtendedQueryResponseData:"));
  
  left = print_debug_uint8(tag, "indoor_fan_pwm", indoor_fan_pwm, left, level);
  left = print_debug_uint8(tag, "indoor_fan_tach", indoor_fan_tach, left, level);
  left = print_debug_enum(tag, "compressor_flags", compressor_flags, left, level);
  left = print_debug_enum(tag, "esp_profile", esp_profile, left, level);
  left = print_debug_enum(tag, "protection_flags", protection_flags, left, level);
  left = coil_inlet_temp.print_debug(tag, "coil_inlet_temp", left, level);
  left = coil_outlet_temp.print_debug(tag, "coil_outlet_temp", left, level);
  left = discharge_temp.print_debug(tag, "discharge_temp", left, level);
  left = print_debug_uint8(tag, "expansion_valve_pos", expansion_valve_pos, left, level);
  left = print_debug_uint8(tag, "reserved1", reserved1, left, level);
  left = print_debug_enum(tag, "system_status_flags", system_status_flags, left, level);
  left = print_debug_uint8(tag, "indoor_unit_address", indoor_unit_address, left, level);
  left = target_temperature.print_debug(tag, "target_temperature", left, level);
  left = compressor_freq_or_fan_rpm.print_debug(tag, "compressor_freq/outdoor_fan_rpm", left, level);
  left = outdoor_temperature.print_debug(tag, "outdoor_temperature", left, level);
  left = print_debug_uint8(tag, "reserved2", reserved2, left, level);
  left = print_debug_uint8(tag, "reserved3", reserved3, left, level);
  left = print_debug_uint8(tag, "static_pressure", static_pressure, left, level);
  left = print_debug_uint8(tag, "reserved4", reserved4, left, level);
  left = print_debug_enum(tag, "subsystem_ok_compressor", subsystem_ok_compressor, left, level);
  left = print_debug_enum(tag, "subsystem_ok_outdoor_fan", subsystem_ok_outdoor_fan, left, level);
  left = print_debug_enum(tag, "subsystem_ok_4way_valve", subsystem_ok_4way_valve, left, level);
  left = print_debug_enum(tag, "subsystem_ok_inverter", subsystem_ok_inverter, left, level);
  
  return left;
}

// ReceiveMessageData methods
size_t ReceiveMessageData::print_debug(const char *tag, size_t left, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ReceiveMessageData (generic):"));
  const uint8_t *ptr = reinterpret_cast<const uint8_t *>(this);
  for (size_t i = 0; i < sizeof(ReceiveMessageData) && left > 0; i++) {
    if (left < 1) return left;
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[%u]: 0x%02X"), i, ptr[i]);
    left -= 1;
  }
  return left;
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

size_t ReceiveData::print_debug(size_t left, const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("RX Message:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame Header:"));
  
  left = print_debug_uint8(tag, "preamble", static_cast<uint8_t>(message.frame.preamble), left, level);
  left = print_debug_enum(tag, "command", message.frame.header.command, left, level);
  left = print_debug_enum(tag, "direction", message.frame.header.direction, left, level);
  left = print_debug_uint8(tag, "destination1", message.frame.header.destination1, left, level);
  left = print_debug_uint8(tag, "source", message.frame.header.source, left, level);
  left = print_debug_uint8(tag, "destination2", message.frame.header.destination2, left, level);
  
  // Delegate to the appropriate data struct's print_debug method based on command type
  switch (message.frame.header.command) {
    case Command::QUERY:
      left = message.data.query_response.print_debug(tag, left, level);
      break;
    
    case Command::QUERY_EXTENDED:
      left = message.data.extended_query_response.print_debug(tag, left, level);
      break;
    
    case Command::SET:
    case Command::FOLLOW_ME:
    case Command::LOCK:
    case Command::UNLOCK:
    default:
      left = message.data.generic.print_debug(tag, left, level);
      break;
  }
  
  // Only print frame end if we have enough bytes left
  // Frame end is at bytes 30-31 in a 32-byte message (RX_MESSAGE_LENGTH)
  if (left >= 2) {  // Check if we have at least 2 bytes left for frame end
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame End:"));
    left = print_debug_uint8(tag, "crc", message.frame_end.crc, left, level);
    left = print_debug_uint8(tag, "prologue", static_cast<uint8_t>(message.frame_end.prologue), left, level);
  }
  
  return left;
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
