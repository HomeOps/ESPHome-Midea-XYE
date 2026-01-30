#ifdef USE_ARDUINO

#include "xye.h"
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

// Temperature methods
float Temperature::to_celsius() const {
  return (value - 0x28) / 2.0f;
}

Temperature Temperature::from_celsius(float celsius) {
  return Temperature{static_cast<uint8_t>(celsius * 2.0f + 0x28)};
}

// Flags16 methods
uint16_t Flags16::value() const {
  return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

void Flags16::set(uint16_t val) {
  low = val & 0xFF;
  high = (val >> 8) & 0xFF;
}

// TransmitMessageData methods
void TransmitMessageData::print_debug(const char *tag, Command command) const {
  switch (command) {
    case Command::SET:
      ESP_LOGD(tag, "  TransmitMessageData (SET):");
      ESP_LOGD(tag, "    operation_mode: 0x%02X", static_cast<uint8_t>(operation_mode));
      ESP_LOGD(tag, "    fan_mode: 0x%02X", static_cast<uint8_t>(fan_mode));
      ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", target_temperature.value, target_temperature.to_celsius());
      ESP_LOGD(tag, "    timer_start: 0x%02X", timer_start);
      ESP_LOGD(tag, "    timer_stop: 0x%02X", timer_stop);
      ESP_LOGD(tag, "    mode_flags: 0x%02X", static_cast<uint8_t>(mode_flags));
      break;
    
    case Command::FOLLOW_ME:
      ESP_LOGD(tag, "  TransmitMessageData (FOLLOW_ME):");
      ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", target_temperature.value, target_temperature.to_celsius());
      ESP_LOGD(tag, "    subcommand (timer_stop): 0x%02X", timer_stop);
      ESP_LOGD(tag, "    mode_flags: 0x%02X", static_cast<uint8_t>(mode_flags));
      break;
    
    case Command::QUERY:
    case Command::QUERY_EXTENDED:
    case Command::LOCK:
    case Command::UNLOCK:
      // These commands typically don't use the data payload
      ESP_LOGD(tag, "  TransmitMessageData (command-only)");
      break;
    
    default:
      ESP_LOGD(tag, "  TransmitMessageData (unknown command)");
      break;
  }
}

// QueryResponseData methods
void QueryResponseData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  QueryResponseData:");
  ESP_LOGD(tag, "    capabilities: 0x%02X", static_cast<uint8_t>(capabilities));
  ESP_LOGD(tag, "    operation_mode: 0x%02X", static_cast<uint8_t>(operation_mode));
  ESP_LOGD(tag, "    fan_mode: 0x%02X", static_cast<uint8_t>(fan_mode));
  ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", target_temperature.value, target_temperature.to_celsius());
  ESP_LOGD(tag, "    t1_temperature: 0x%02X (%.1f°C)", t1_temperature.value, t1_temperature.to_celsius());
  ESP_LOGD(tag, "    t2a_temperature: 0x%02X (%.1f°C)", t2a_temperature.value, t2a_temperature.to_celsius());
  ESP_LOGD(tag, "    t2b_temperature: 0x%02X (%.1f°C)", t2b_temperature.value, t2b_temperature.to_celsius());
  ESP_LOGD(tag, "    t3_temperature: 0x%02X (%.1f°C)", t3_temperature.value, t3_temperature.to_celsius());
  ESP_LOGD(tag, "    current: %d", current);
  ESP_LOGD(tag, "    timer_start: 0x%02X", timer_start);
  ESP_LOGD(tag, "    timer_stop: 0x%02X", timer_stop);
  ESP_LOGD(tag, "    mode_flags: 0x%02X", static_cast<uint8_t>(mode_flags));
  ESP_LOGD(tag, "    operation_flags: 0x%02X", static_cast<uint8_t>(operation_flags));
  ESP_LOGD(tag, "    error_flags: 0x%04X", error_flags.value());
  ESP_LOGD(tag, "    protect_flags: 0x%04X", protect_flags.value());
  ESP_LOGD(tag, "    ccm_communication_error_flags: 0x%02X", static_cast<uint8_t>(ccm_communication_error_flags));
}

// ExtendedQueryResponseData methods
void ExtendedQueryResponseData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  ExtendedQueryResponseData:");
  ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", target_temperature.value, target_temperature.to_celsius());
  ESP_LOGD(tag, "    outdoor_temperature: 0x%02X (%.1f°C)", outdoor_temperature.value, outdoor_temperature.to_celsius());
  ESP_LOGD(tag, "    static_pressure: 0x%02X", static_pressure);
}

// ReceiveMessageData methods
void ReceiveMessageData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "  ReceiveMessageData (generic):");
  ESP_LOGD(tag, "    Raw data (24 bytes): %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
           data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
           data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23]);
}

// TransmitData methods
void TransmitData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "TX Message:");
  ESP_LOGD(tag, "  Command: 0x%02X", static_cast<uint8_t>(message.frame.header.command));
  
  // Delegate to the data struct's print_debug method
  message.data.standard.print_debug(tag, message.frame.header.command);
  
  ESP_LOGD(tag, "  Raw: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
           raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
           raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
}

// ReceiveData methods
Command ReceiveData::get_command() const {
  return message.frame.header.command;
}

void ReceiveData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "RX Message:");
  ESP_LOGD(tag, "  Command: 0x%02X", static_cast<uint8_t>(message.frame.header.command));
  
  // Delegate to the appropriate data struct's print_debug method based on command type
  switch (message.frame.header.command) {
    case Command::QUERY:
      message.data.query_response.print_debug(tag);
      break;
    
    case Command::QUERY_EXTENDED:
      message.data.extended_query_response.print_debug(tag);
      break;
    
    case Command::SET:
      ESP_LOGD(tag, "  Set Response");
      message.data.generic.print_debug(tag);
      break;
    
    case Command::FOLLOW_ME:
      ESP_LOGD(tag, "  Follow-Me Response");
      message.data.generic.print_debug(tag);
      break;
    
    default:
      ESP_LOGD(tag, "  Unknown command type");
      message.data.generic.print_debug(tag);
      break;
  }
  
  ESP_LOGD(tag, "  Raw: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
           raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
           raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15],
           raw[16], raw[17], raw[18], raw[19], raw[20], raw[21], raw[22], raw[23],
           raw[24], raw[25], raw[26], raw[27], raw[28], raw[29], raw[30], raw[31]);
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
