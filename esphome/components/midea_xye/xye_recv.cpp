#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// QueryResponseData methods
void QueryResponseData::print_debug(const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  QueryResponseData:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown1: 0x%02X"), unknown1);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    capabilities: 0x%02X (%s)"), 
           static_cast<uint8_t>(capabilities),
           enum_to_string(capabilities));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    operation_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    fan_mode: 0x%02X (%s)"), 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode));
  target_temperature.print_debug(tag, "target_temperature", level);
  t1_temperature.print_debug(tag, "t1_temperature", level);
  t2a_temperature.print_debug(tag, "t2a_temperature", level);
  t2b_temperature.print_debug(tag, "t2b_temperature", level);
  t3_temperature.print_debug(tag, "t3_temperature", level);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    current: 0x%02X"), current);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown2: 0x%02X"), unknown2);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_start: 0x%02X"), timer_start);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    timer_stop: 0x%02X"), timer_stop);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown3: 0x%02X"), unknown3);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    mode_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    operation_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(operation_flags),
           enum_to_string(operation_flags));
  error_flags.print_debug(tag, "error_flags", level);
  protect_flags.print_debug(tag, "protect_flags", level);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    ccm_communication_error_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(ccm_communication_error_flags),
           enum_to_string(ccm_communication_error_flags));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown4: 0x%02X"), unknown4);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown5: 0x%02X"), unknown5);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    unknown6: 0x%02X"), unknown6);
}

// ExtendedQueryResponseData methods
void ExtendedQueryResponseData::print_debug(const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ExtendedQueryResponseData:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_fan_pwm: 0x%02X"), indoor_fan_pwm);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_fan_tach: 0x%02X"), indoor_fan_tach);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    compressor_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(compressor_flags),
           enum_to_string(compressor_flags));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    esp_profile: 0x%02X (%s)"), 
           static_cast<uint8_t>(esp_profile),
           enum_to_string(esp_profile));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    protection_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(protection_flags),
           enum_to_string(protection_flags));
  coil_inlet_temp.print_debug(tag, "coil_inlet_temp", level);
  coil_outlet_temp.print_debug(tag, "coil_outlet_temp", level);
  discharge_temp.print_debug(tag, "discharge_temp", level);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    expansion_valve_pos: 0x%02X"), expansion_valve_pos);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved1: 0x%02X"), reserved1);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    system_status_flags: 0x%02X (%s)"), 
           static_cast<uint8_t>(system_status_flags),
           enum_to_string(system_status_flags));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    indoor_unit_address: 0x%02X"), indoor_unit_address);
  target_temperature.print_debug(tag, "target_temperature", level);
  compressor_freq_or_fan_rpm.print_debug(tag, "compressor_freq/outdoor_fan_rpm", level);
  outdoor_temperature.print_debug(tag, "outdoor_temperature", level);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved2: 0x%02X"), reserved2);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved3: 0x%02X"), reserved3);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    static_pressure: 0x%02X"), static_pressure);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    reserved4: 0x%02X"), reserved4);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_compressor: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_compressor),
           enum_to_string(subsystem_ok_compressor));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_outdoor_fan: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_outdoor_fan),
           enum_to_string(subsystem_ok_outdoor_fan));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_4way_valve: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_4way_valve),
           enum_to_string(subsystem_ok_4way_valve));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    subsystem_ok_inverter: 0x%02X (%s)"), 
           static_cast<uint8_t>(subsystem_ok_inverter),
           enum_to_string(subsystem_ok_inverter));
}

// ReceiveMessageData methods
void ReceiveMessageData::print_debug(const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  ReceiveMessageData (generic):"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[0]: 0x%02X"), data[0]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[1]: 0x%02X"), data[1]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[2]: 0x%02X"), data[2]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[3]: 0x%02X"), data[3]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[4]: 0x%02X"), data[4]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[5]: 0x%02X"), data[5]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[6]: 0x%02X"), data[6]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[7]: 0x%02X"), data[7]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[8]: 0x%02X"), data[8]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[9]: 0x%02X"), data[9]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[10]: 0x%02X"), data[10]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[11]: 0x%02X"), data[11]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[12]: 0x%02X"), data[12]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[13]: 0x%02X"), data[13]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[14]: 0x%02X"), data[14]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[15]: 0x%02X"), data[15]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[16]: 0x%02X"), data[16]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[17]: 0x%02X"), data[17]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[18]: 0x%02X"), data[18]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[19]: 0x%02X"), data[19]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[20]: 0x%02X"), data[20]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[21]: 0x%02X"), data[21]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[22]: 0x%02X"), data[22]);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    byte[23]: 0x%02X"), data[23]);
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

void ReceiveData::print_debug(const char *tag, int level) const {
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("RX Message:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame Header:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    preamble: 0x%02X"), static_cast<uint8_t>(message.frame.preamble));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    command: 0x%02X (%s)"), 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    direction: 0x%02X (%s)"), 
           static_cast<uint8_t>(message.frame.header.direction),
           enum_to_string(message.frame.header.direction));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    destination1: 0x%02X"), message.frame.header.destination1);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    source: 0x%02X"), message.frame.header.source);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    destination2: 0x%02X"), message.frame.header.destination2);
  
  // Delegate to the appropriate data struct's print_debug method based on command type
  switch (message.frame.header.command) {
    case Command::QUERY:
      message.data.query_response.print_debug(tag, level);
      break;
    
    case Command::QUERY_EXTENDED:
      message.data.extended_query_response.print_debug(tag, level);
      break;
    
    case Command::SET:
    case Command::FOLLOW_ME:
    case Command::LOCK:
    case Command::UNLOCK:
    default:
      message.data.generic.print_debug(tag, level);
      break;
  }
  
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("  Frame End:"));
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    crc: 0x%02X"), message.frame_end.crc);
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    prologue: 0x%02X"), static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
