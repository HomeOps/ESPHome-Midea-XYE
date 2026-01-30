#ifdef USE_ARDUINO

#include "xye_recv.h"
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

// External enum-to-string maps
extern const std::map<Command, const char*> COMMAND_MAP;
extern const std::map<OperationMode, const char*> OPERATION_MODE_MAP;
extern const std::map<FanMode, const char*> FAN_MODE_MAP;
extern const std::map<ModeFlags, const char*> MODE_FLAGS_MAP;
extern const std::map<OperationFlags, const char*> OPERATION_FLAGS_MAP;
extern const std::map<Capabilities, const char*> CAPABILITIES_MAP;
extern const std::map<Direction, const char*> DIRECTION_MAP;
extern const std::map<CcmErrorFlags, const char*> CCM_ERROR_FLAGS_MAP;

// QueryResponseData methods
void QueryResponseData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  QueryResponseData:");
  ESP_LOGD(tag, "    unknown1: 0x%02X", unknown1);
  ESP_LOGD(tag, "    capabilities: 0x%02X (%s)", 
           static_cast<uint8_t>(capabilities),
           enum_to_string(capabilities, CAPABILITIES_MAP));
  ESP_LOGD(tag, "    operation_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode, OPERATION_MODE_MAP));
  ESP_LOGD(tag, "    fan_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode, FAN_MODE_MAP));
  ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  ESP_LOGD(tag, "    t1_temperature: 0x%02X (%.1f°C)", 
           t1_temperature.value, 
           t1_temperature.to_celsius());
  ESP_LOGD(tag, "    t2a_temperature: 0x%02X (%.1f°C)", 
           t2a_temperature.value, 
           t2a_temperature.to_celsius());
  ESP_LOGD(tag, "    t2b_temperature: 0x%02X (%.1f°C)", 
           t2b_temperature.value, 
           t2b_temperature.to_celsius());
  ESP_LOGD(tag, "    t3_temperature: 0x%02X (%.1f°C)", 
           t3_temperature.value, 
           t3_temperature.to_celsius());
  ESP_LOGD(tag, "    current: 0x%02X", current);
  ESP_LOGD(tag, "    unknown2: 0x%02X", unknown2);
  ESP_LOGD(tag, "    timer_start: 0x%02X", timer_start);
  ESP_LOGD(tag, "    timer_stop: 0x%02X", timer_stop);
  ESP_LOGD(tag, "    unknown3: 0x%02X", unknown3);
  ESP_LOGD(tag, "    mode_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags, MODE_FLAGS_MAP));
  ESP_LOGD(tag, "    operation_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_flags),
           enum_to_string(operation_flags, OPERATION_FLAGS_MAP));
  ESP_LOGD(tag, "    error_flags: 0x%04X", error_flags.value());
  ESP_LOGD(tag, "    protect_flags: 0x%04X", protect_flags.value());
  ESP_LOGD(tag, "    ccm_communication_error_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(ccm_communication_error_flags),
           enum_to_string(ccm_communication_error_flags, CCM_ERROR_FLAGS_MAP));
  ESP_LOGD(tag, "    unknown4: 0x%02X", unknown4);
  ESP_LOGD(tag, "    unknown5: 0x%02X", unknown5);
  ESP_LOGD(tag, "    unknown6: 0x%02X", unknown6);
}

// ExtendedQueryResponseData methods
void ExtendedQueryResponseData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  ExtendedQueryResponseData:");
  ESP_LOGD(tag, "    unknown1: 0x%02X", unknown1);
  ESP_LOGD(tag, "    unknown2: 0x%02X", unknown2);
  ESP_LOGD(tag, "    unknown3: 0x%02X", unknown3);
  ESP_LOGD(tag, "    unknown4: 0x%02X", unknown4);
  ESP_LOGD(tag, "    unknown5: 0x%02X", unknown5);
  ESP_LOGD(tag, "    unknown6: 0x%02X", unknown6);
  ESP_LOGD(tag, "    unknown7: 0x%02X", unknown7);
  ESP_LOGD(tag, "    unknown8: 0x%02X", unknown8);
  ESP_LOGD(tag, "    unknown9: 0x%02X", unknown9);
  ESP_LOGD(tag, "    unknown10: 0x%02X", unknown10);
  ESP_LOGD(tag, "    unknown11: 0x%02X", unknown11);
  ESP_LOGD(tag, "    unknown12: 0x%02X", unknown12);
  ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  ESP_LOGD(tag, "    unknown13: 0x%02X", unknown13);
  ESP_LOGD(tag, "    unknown14: 0x%02X", unknown14);
  ESP_LOGD(tag, "    outdoor_temperature: 0x%02X (%.1f°C)", 
           outdoor_temperature.value, 
           outdoor_temperature.to_celsius());
  ESP_LOGD(tag, "    unknown16: 0x%02X", unknown16);
  ESP_LOGD(tag, "    unknown17: 0x%02X", unknown17);
  ESP_LOGD(tag, "    static_pressure: 0x%02X", static_pressure);
  ESP_LOGD(tag, "    unknown18: 0x%02X", unknown18);
  ESP_LOGD(tag, "    unknown19: 0x%02X", unknown19);
  ESP_LOGD(tag, "    unknown20: 0x%02X", unknown20);
  ESP_LOGD(tag, "    unknown21: 0x%02X", unknown21);
  ESP_LOGD(tag, "    unknown22: 0x%02X", unknown22);
}

// ReceiveMessageData methods
void ReceiveMessageData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  ReceiveMessageData (generic):");
  ESP_LOGD(tag, "    byte[0]: 0x%02X", data[0]);
  ESP_LOGD(tag, "    byte[1]: 0x%02X", data[1]);
  ESP_LOGD(tag, "    byte[2]: 0x%02X", data[2]);
  ESP_LOGD(tag, "    byte[3]: 0x%02X", data[3]);
  ESP_LOGD(tag, "    byte[4]: 0x%02X", data[4]);
  ESP_LOGD(tag, "    byte[5]: 0x%02X", data[5]);
  ESP_LOGD(tag, "    byte[6]: 0x%02X", data[6]);
  ESP_LOGD(tag, "    byte[7]: 0x%02X", data[7]);
  ESP_LOGD(tag, "    byte[8]: 0x%02X", data[8]);
  ESP_LOGD(tag, "    byte[9]: 0x%02X", data[9]);
  ESP_LOGD(tag, "    byte[10]: 0x%02X", data[10]);
  ESP_LOGD(tag, "    byte[11]: 0x%02X", data[11]);
  ESP_LOGD(tag, "    byte[12]: 0x%02X", data[12]);
  ESP_LOGD(tag, "    byte[13]: 0x%02X", data[13]);
  ESP_LOGD(tag, "    byte[14]: 0x%02X", data[14]);
  ESP_LOGD(tag, "    byte[15]: 0x%02X", data[15]);
  ESP_LOGD(tag, "    byte[16]: 0x%02X", data[16]);
  ESP_LOGD(tag, "    byte[17]: 0x%02X", data[17]);
  ESP_LOGD(tag, "    byte[18]: 0x%02X", data[18]);
  ESP_LOGD(tag, "    byte[19]: 0x%02X", data[19]);
  ESP_LOGD(tag, "    byte[20]: 0x%02X", data[20]);
  ESP_LOGD(tag, "    byte[21]: 0x%02X", data[21]);
  ESP_LOGD(tag, "    byte[22]: 0x%02X", data[22]);
  ESP_LOGD(tag, "    byte[23]: 0x%02X", data[23]);
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

void ReceiveData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "RX Message:");
  ESP_LOGD(tag, "  Frame Header:");
  ESP_LOGD(tag, "    preamble: 0x%02X", static_cast<uint8_t>(message.frame.preamble));
  ESP_LOGD(tag, "    command: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command, COMMAND_MAP));
  ESP_LOGD(tag, "    direction: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.direction),
           enum_to_string(message.frame.header.direction, DIRECTION_MAP));
  ESP_LOGD(tag, "    destination1: 0x%02X", message.frame.header.destination1);
  ESP_LOGD(tag, "    source: 0x%02X", message.frame.header.source);
  ESP_LOGD(tag, "    destination2: 0x%02X", message.frame.header.destination2);
  
  // Delegate to the appropriate data struct's print_debug method based on command type
  switch (message.frame.header.command) {
    case Command::QUERY:
      message.data.query_response.print_debug(tag);
      break;
    
    case Command::QUERY_EXTENDED:
      message.data.extended_query_response.print_debug(tag);
      break;
    
    case Command::SET:
    case Command::FOLLOW_ME:
    case Command::LOCK:
    case Command::UNLOCK:
    default:
      message.data.generic.print_debug(tag);
      break;
  }
  
  ESP_LOGD(tag, "  Frame End:");
  ESP_LOGD(tag, "    crc: 0x%02X", message.frame_end.crc);
  ESP_LOGD(tag, "    prologue: 0x%02X", static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
