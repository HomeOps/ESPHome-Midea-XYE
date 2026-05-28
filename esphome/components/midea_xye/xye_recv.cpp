#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "xye_adapter.h"
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
  left = target_temperature.print_debug(tag, "target_temperature", left, level, TemperatureEncoding::RAW);
  left = t1_temperature.print_debug(tag, "t1_temperature", left, level);
  left = t2a_temperature.print_debug(tag, "t2a_temperature", left, level);
  left = t2b_temperature.print_debug(tag, "t2b_temperature", left, level);
  left = t3_temperature.print_debug(tag, "t3_temperature", left, level);
  left = print_debug_uint8(tag, "current", current, left, level);
  left = print_debug_uint8(tag, "unknown2", unknown2, left, level);
  left = print_debug_uint8(tag, "timer_start", timer_start, left, level);
  left = print_debug_uint8(tag, "timer_stop", timer_stop, left, level);
  left = print_debug_enum(tag, "compressor_running_flag", compressor_running_flag, left, level);
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
static size_t print_setpoint_debug(const char *tag, const char *name, uint8_t raw,
                                   size_t left, int level, bool use_fahrenheit) {
  if (left < sizeof(Temperature)) return left;
  const float celsius = XYEAdapter::get_target_temperature(raw, use_fahrenheit);
  if (use_fahrenheit) {
    const int fahrenheit = static_cast<int>(raw) - static_cast<int>(FAHRENHEIT_TEMP_OFFSET);
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%02X (%d°F / %.2f°C)"),
             name, raw, fahrenheit, celsius);
  } else {
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%02X (%.2f°C)"),
             name, raw, celsius);
  }
  return left - sizeof(Temperature);
}

size_t ExtendedQueryResponseData::print_debug(const char *tag, size_t left, int level, bool use_fahrenheit) const {
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
  left = print_debug_enum(tag, "target_fan_speed", target_fan_speed, left, level);
  left = print_setpoint_debug(tag, "target_temperature", target_temperature.value, left, level, use_fahrenheit);
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

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

size_t ReceiveData::print_debug(size_t left, const char *tag, int level, bool use_fahrenheit) const {
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
      left = message.data.extended_query_response.print_debug(tag, left, level, use_fahrenheit);
      break;
    
    case Command::SET:
      left = message.data.set_response.print_debug(tag, left, level);
      break;
    
    case Command::FOLLOW_ME:
      left = message.data.follow_me_response.print_debug(tag, left, level);
      break;
    
    case Command::LOCK:
      left = message.data.lock_response.print_debug(tag, left, level);
      break;
    
    case Command::UNLOCK:
      left = message.data.unlock_response.print_debug(tag, left, level);
      break;
    
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

bool ReceiveData::is_valid() const noexcept {
  return message.frame.preamble == ProtocolMarker::PREAMBLE &&
         message.frame_end.prologue == ProtocolMarker::PROLOGUE &&
         message.frame.header.direction == Direction::TO_CLIENT &&
         message.frame_end.crc == compute_protocol_crc(raw, RX_MESSAGE_LENGTH);
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
