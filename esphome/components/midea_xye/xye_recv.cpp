#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// QueryResponseData methods
size_t QueryResponseData::print_debug(const char *tag, size_t bytes_remaining, int level) const {
  size_t consumed = 0;
  size_t left = bytes_remaining;
  size_t n;
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  QueryResponseData:"));
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown1: 0x%02X"), unknown1);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    capabilities: 0x%02X (%s)"), 
           static_cast<uint8_t>(capabilities),
           enum_to_string(capabilities));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    operation_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    fan_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode));
  left -= 1; consumed += 1;
  
  n = target_temperature.print_debug(tag, "target_temperature", left, level);
  left -= n; consumed += n;
  n = t1_temperature.print_debug(tag, "t1_temperature", left, level);
  left -= n; consumed += n;
  n = t2a_temperature.print_debug(tag, "t2a_temperature", left, level);
  left -= n; consumed += n;
  n = t2b_temperature.print_debug(tag, "t2b_temperature", left, level);
  left -= n; consumed += n;
  n = t3_temperature.print_debug(tag, "t3_temperature", left, level);
  left -= n; consumed += n;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    current: 0x%02X"), current);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown2: 0x%02X"), unknown2);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_start: 0x%02X"), timer_start);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_stop: 0x%02X"), timer_stop);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown3: 0x%02X"), unknown3);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    mode_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    operation_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(operation_flags),
           enum_to_string(operation_flags));
  left -= 1; consumed += 1;
  
  n = error_flags.print_debug(tag, "error_flags", left, level);
  left -= n; consumed += n;
  n = protect_flags.print_debug(tag, "protect_flags", left, level);
  left -= n; consumed += n;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    ccm_communication_error_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(ccm_communication_error_flags),
           enum_to_string(ccm_communication_error_flags));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown4: 0x%02X"), unknown4);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown5: 0x%02X"), unknown5);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown6: 0x%02X"), unknown6);
  left -= 1; consumed += 1;
  
  return consumed;
}

// ExtendedQueryResponseData methods
size_t ExtendedQueryResponseData::print_debug(const char *tag, size_t bytes_remaining, int level) const {
  size_t consumed = 0;
  size_t left = bytes_remaining;
  size_t n;
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ExtendedQueryResponseData:"));
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_fan_pwm: 0x%02X"), indoor_fan_pwm);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_fan_tach: 0x%02X"), indoor_fan_tach);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    compressor_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(compressor_flags),
           enum_to_string(compressor_flags));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    esp_profile: 0x%02X (%s)"), 
           static_cast<uint8_t>(esp_profile),
           enum_to_string(esp_profile));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    protection_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(protection_flags),
           enum_to_string(protection_flags));
  left -= 1; consumed += 1;
  
  n = coil_inlet_temp.print_debug(tag, "coil_inlet_temp", left, level);
  left -= n; consumed += n;
  n = coil_outlet_temp.print_debug(tag, "coil_outlet_temp", left, level);
  left -= n; consumed += n;
  n = discharge_temp.print_debug(tag, "discharge_temp", left, level);
  left -= n; consumed += n;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    expansion_valve_pos: 0x%02X"), expansion_valve_pos);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved1: 0x%02X"), reserved1);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    system_status_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(system_status_flags),
           enum_to_string(system_status_flags));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_unit_address: 0x%02X"), indoor_unit_address);
  left -= 1; consumed += 1;
  
  n = target_temperature.print_debug(tag, "target_temperature", left, level);
  left -= n; consumed += n;
  n = compressor_freq_or_fan_rpm.print_debug(tag, "compressor_freq/outdoor_fan_rpm", left, level);
  left -= n; consumed += n;
  n = outdoor_temperature.print_debug(tag, "outdoor_temperature", left, level);
  left -= n; consumed += n;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved2: 0x%02X"), reserved2);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved3: 0x%02X"), reserved3);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    static_pressure: 0x%02X"), static_pressure);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved4: 0x%02X"), reserved4);
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_compressor: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_compressor),
           enum_to_string(subsystem_ok_compressor));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_outdoor_fan: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_outdoor_fan),
           enum_to_string(subsystem_ok_outdoor_fan));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_4way_valve: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_4way_valve),
           enum_to_string(subsystem_ok_4way_valve));
  left -= 1; consumed += 1;
  
  if (left < 1) return consumed;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_inverter: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_inverter),
           enum_to_string(subsystem_ok_inverter));
  left -= 1; consumed += 1;
  
  return consumed;
}

// ReceiveMessageData methods
size_t ReceiveMessageData::print_debug(const char *tag, size_t bytes_remaining, int level) const {
  size_t consumed = 0;
  size_t left = bytes_remaining;
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ReceiveMessageData (generic):"));
  for (size_t i = 0; i < 24 && i < left; i++) {
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[%u]: 0x%02X"), i, data[i]);
    consumed++;
  }
  return consumed;
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

void ReceiveData::print_debug(const char *tag, size_t received_size, int level) const {
  // Start with bytes_remaining equal to received_size
  // Subtract the frame header size (6 bytes)
  size_t left = received_size;
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("RX Message:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame Header:"));
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    preamble: 0x%02X"), static_cast<uint8_t>(message.frame.preamble));
  left -= 1;
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    command: 0x%02X (%s)"), 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command));
  left -= 1;
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    direction: 0x%02X (%s)"), 
           static_cast<uint8_t>(message.frame.header.direction),
           enum_to_string(message.frame.header.direction));
  left -= 1;
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    destination1: 0x%02X"), message.frame.header.destination1);
  left -= 1;
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    source: 0x%02X"), message.frame.header.source);
  left -= 1;
  
  if (left < 1) return;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    destination2: 0x%02X"), message.frame.header.destination2);
  left -= 1;
  
  // Delegate to the appropriate data struct's print_debug method based on command type
  switch (message.frame.header.command) {
    case Command::QUERY:
      left -= message.data.query_response.print_debug(tag, left, level);
      break;
    
    case Command::QUERY_EXTENDED:
      left -= message.data.extended_query_response.print_debug(tag, left, level);
      break;
    
    case Command::SET:
    case Command::FOLLOW_ME:
    case Command::LOCK:
    case Command::UNLOCK:
    default:
      left -= message.data.generic.print_debug(tag, left, level);
      break;
  }
  
  // Only print frame end if we have enough bytes
  // Frame end is at bytes 30-31 in a 32-byte message
  if (received_size >= 31) {
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame End:"));
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    crc: 0x%02X"), message.frame_end.crc);
  }
  if (received_size >= 32) {
    ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    prologue: 0x%02X"), static_cast<uint8_t>(message.frame_end.prologue));
  }
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
