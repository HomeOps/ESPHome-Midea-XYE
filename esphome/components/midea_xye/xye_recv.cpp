#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "xye_log.h"

namespace esphome {
namespace midea {
namespace xye {

// QueryResponseData methods
void QueryResponseData::print_debug(const char *tag, int level) const {
  ESP_LOG_LEVEL(level, tag, "  QueryResponseData:");
  ESP_LOG_LEVEL(level, tag, "    unknown1: 0x%02X", unknown1);
  ESP_LOG_LEVEL(level, tag, "    capabilities: 0x%02X (%s)", 
           static_cast<uint8_t>(capabilities),
           enum_to_string(capabilities));
  ESP_LOG_LEVEL(level, tag, "    operation_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode));
  ESP_LOG_LEVEL(level, tag, "    fan_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode));
  ESP_LOG_LEVEL(level, tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    t1_temperature: 0x%02X (%.1f°C)", 
           t1_temperature.value, 
           t1_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    t2a_temperature: 0x%02X (%.1f°C)", 
           t2a_temperature.value, 
           t2a_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    t2b_temperature: 0x%02X (%.1f°C)", 
           t2b_temperature.value, 
           t2b_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    t3_temperature: 0x%02X (%.1f°C)", 
           t3_temperature.value, 
           t3_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    current: 0x%02X", current);
  ESP_LOG_LEVEL(level, tag, "    unknown2: 0x%02X", unknown2);
  ESP_LOG_LEVEL(level, tag, "    timer_start: 0x%02X", timer_start);
  ESP_LOG_LEVEL(level, tag, "    timer_stop: 0x%02X", timer_stop);
  ESP_LOG_LEVEL(level, tag, "    unknown3: 0x%02X", unknown3);
  ESP_LOG_LEVEL(level, tag, "    mode_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags));
  ESP_LOG_LEVEL(level, tag, "    operation_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_flags),
           enum_to_string(operation_flags));
  ESP_LOG_LEVEL(level, tag, "    error_flags: 0x%04X", error_flags.value());
  ESP_LOG_LEVEL(level, tag, "    protect_flags: 0x%04X", protect_flags.value());
  ESP_LOG_LEVEL(level, tag, "    ccm_communication_error_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(ccm_communication_error_flags),
           enum_to_string(ccm_communication_error_flags));
  ESP_LOG_LEVEL(level, tag, "    unknown4: 0x%02X", unknown4);
  ESP_LOG_LEVEL(level, tag, "    unknown5: 0x%02X", unknown5);
  ESP_LOG_LEVEL(level, tag, "    unknown6: 0x%02X", unknown6);
}

// ExtendedQueryResponseData methods
void ExtendedQueryResponseData::print_debug(const char *tag, int level) const {
  ESP_LOG_LEVEL(level, tag, "  ExtendedQueryResponseData:");
  ESP_LOG_LEVEL(level, tag, "    unknown1: 0x%02X", unknown1);
  ESP_LOG_LEVEL(level, tag, "    unknown2: 0x%02X", unknown2);
  ESP_LOG_LEVEL(level, tag, "    unknown3: 0x%02X", unknown3);
  ESP_LOG_LEVEL(level, tag, "    unknown4: 0x%02X", unknown4);
  ESP_LOG_LEVEL(level, tag, "    unknown5: 0x%02X", unknown5);
  ESP_LOG_LEVEL(level, tag, "    unknown6: 0x%02X", unknown6);
  ESP_LOG_LEVEL(level, tag, "    unknown7: 0x%02X", unknown7);
  ESP_LOG_LEVEL(level, tag, "    unknown8: 0x%02X", unknown8);
  ESP_LOG_LEVEL(level, tag, "    unknown9: 0x%02X", unknown9);
  ESP_LOG_LEVEL(level, tag, "    unknown10: 0x%02X", unknown10);
  ESP_LOG_LEVEL(level, tag, "    unknown11: 0x%02X", unknown11);
  ESP_LOG_LEVEL(level, tag, "    unknown12: 0x%02X", unknown12);
  ESP_LOG_LEVEL(level, tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    unknown13: 0x%02X", unknown13);
  ESP_LOG_LEVEL(level, tag, "    unknown14: 0x%02X", unknown14);
  ESP_LOG_LEVEL(level, tag, "    outdoor_temperature: 0x%02X (%.1f°C)", 
           outdoor_temperature.value, 
           outdoor_temperature.to_celsius());
  ESP_LOG_LEVEL(level, tag, "    unknown16: 0x%02X", unknown16);
  ESP_LOG_LEVEL(level, tag, "    unknown17: 0x%02X", unknown17);
  ESP_LOG_LEVEL(level, tag, "    static_pressure: 0x%02X", static_pressure);
  ESP_LOG_LEVEL(level, tag, "    unknown18: 0x%02X", unknown18);
  ESP_LOG_LEVEL(level, tag, "    unknown19: 0x%02X", unknown19);
  ESP_LOG_LEVEL(level, tag, "    unknown20: 0x%02X", unknown20);
  ESP_LOG_LEVEL(level, tag, "    unknown21: 0x%02X", unknown21);
  ESP_LOG_LEVEL(level, tag, "    unknown22: 0x%02X", unknown22);
}

// ReceiveMessageData methods
void ReceiveMessageData::print_debug(const char *tag, int level) const {
  ESP_LOG_LEVEL(level, tag, "  ReceiveMessageData (generic):");
  ESP_LOG_LEVEL(level, tag, "    byte[0]: 0x%02X", data[0]);
  ESP_LOG_LEVEL(level, tag, "    byte[1]: 0x%02X", data[1]);
  ESP_LOG_LEVEL(level, tag, "    byte[2]: 0x%02X", data[2]);
  ESP_LOG_LEVEL(level, tag, "    byte[3]: 0x%02X", data[3]);
  ESP_LOG_LEVEL(level, tag, "    byte[4]: 0x%02X", data[4]);
  ESP_LOG_LEVEL(level, tag, "    byte[5]: 0x%02X", data[5]);
  ESP_LOG_LEVEL(level, tag, "    byte[6]: 0x%02X", data[6]);
  ESP_LOG_LEVEL(level, tag, "    byte[7]: 0x%02X", data[7]);
  ESP_LOG_LEVEL(level, tag, "    byte[8]: 0x%02X", data[8]);
  ESP_LOG_LEVEL(level, tag, "    byte[9]: 0x%02X", data[9]);
  ESP_LOG_LEVEL(level, tag, "    byte[10]: 0x%02X", data[10]);
  ESP_LOG_LEVEL(level, tag, "    byte[11]: 0x%02X", data[11]);
  ESP_LOG_LEVEL(level, tag, "    byte[12]: 0x%02X", data[12]);
  ESP_LOG_LEVEL(level, tag, "    byte[13]: 0x%02X", data[13]);
  ESP_LOG_LEVEL(level, tag, "    byte[14]: 0x%02X", data[14]);
  ESP_LOG_LEVEL(level, tag, "    byte[15]: 0x%02X", data[15]);
  ESP_LOG_LEVEL(level, tag, "    byte[16]: 0x%02X", data[16]);
  ESP_LOG_LEVEL(level, tag, "    byte[17]: 0x%02X", data[17]);
  ESP_LOG_LEVEL(level, tag, "    byte[18]: 0x%02X", data[18]);
  ESP_LOG_LEVEL(level, tag, "    byte[19]: 0x%02X", data[19]);
  ESP_LOG_LEVEL(level, tag, "    byte[20]: 0x%02X", data[20]);
  ESP_LOG_LEVEL(level, tag, "    byte[21]: 0x%02X", data[21]);
  ESP_LOG_LEVEL(level, tag, "    byte[22]: 0x%02X", data[22]);
  ESP_LOG_LEVEL(level, tag, "    byte[23]: 0x%02X", data[23]);
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

void ReceiveData::print_debug(const char *tag, int level) const {
  ESP_LOG_LEVEL(level, tag, "RX Message:");
  ESP_LOG_LEVEL(level, tag, "  Frame Header:");
  ESP_LOG_LEVEL(level, tag, "    preamble: 0x%02X", static_cast<uint8_t>(message.frame.preamble));
  ESP_LOG_LEVEL(level, tag, "    command: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command));
  ESP_LOG_LEVEL(level, tag, "    direction: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.direction),
           enum_to_string(message.frame.header.direction));
  ESP_LOG_LEVEL(level, tag, "    destination1: 0x%02X", message.frame.header.destination1);
  ESP_LOG_LEVEL(level, tag, "    source: 0x%02X", message.frame.header.source);
  ESP_LOG_LEVEL(level, tag, "    destination2: 0x%02X", message.frame.header.destination2);
  
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
  
  ESP_LOG_LEVEL(level, tag, "  Frame End:");
  ESP_LOG_LEVEL(level, tag, "    crc: 0x%02X", message.frame_end.crc);
  ESP_LOG_LEVEL(level, tag, "    prologue: 0x%02X", static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
