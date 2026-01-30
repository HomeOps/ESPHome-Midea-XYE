#ifdef USE_ARDUINO

#include "xye_send.h"
#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

// External enum-to-string maps
extern const std::map<Command, const char*> COMMAND_MAP;
extern const std::map<OperationMode, const char*> OPERATION_MODE_MAP;
extern const std::map<FanMode, const char*> FAN_MODE_MAP;
extern const std::map<ModeFlags, const char*> MODE_FLAGS_MAP;
extern const std::map<FollowMeSubcommand, const char*> FOLLOW_ME_SUBCOMMAND_MAP;

// TransmitMessageData methods
void TransmitMessageData::print_debug(const char *tag, Command command) const {
  ESP_LOGD(tag, "  TransmitMessageData:");
  ESP_LOGD(tag, "    operation_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(operation_mode),
           enum_to_string(operation_mode, OPERATION_MODE_MAP));
  ESP_LOGD(tag, "    fan_mode: 0x%02X (%s)", 
           static_cast<uint8_t>(fan_mode),
           enum_to_string(fan_mode, FAN_MODE_MAP));
  ESP_LOGD(tag, "    target_temperature: 0x%02X (%.1f°C)", 
           target_temperature.value, 
           target_temperature.to_celsius());
  ESP_LOGD(tag, "    timer_start: 0x%02X", timer_start);
  ESP_LOGD(tag, "    timer_stop: 0x%02X", timer_stop);
  ESP_LOGD(tag, "    mode_flags: 0x%02X (%s)", 
           static_cast<uint8_t>(mode_flags),
           enum_to_string(mode_flags, MODE_FLAGS_MAP));
  ESP_LOGD(tag, "    reserved1: 0x%02X", reserved1);
  ESP_LOGD(tag, "    complement: 0x%02X", complement);
}

// TransmitData methods
void TransmitData::print_debug(const char *tag) const {
  ESP_LOGD(tag, "TX Message:");
  ESP_LOGD(tag, "  Frame Header:");
  ESP_LOGD(tag, "    preamble: 0x%02X", static_cast<uint8_t>(message.frame.preamble));
  ESP_LOGD(tag, "    command: 0x%02X (%s)", 
           static_cast<uint8_t>(message.frame.header.command),
           enum_to_string(message.frame.header.command, COMMAND_MAP));
  ESP_LOGD(tag, "    server_id: 0x%02X", message.frame.header.server_id);
  ESP_LOGD(tag, "    client_id1: 0x%02X", message.frame.header.client_id1);
  ESP_LOGD(tag, "    direction: 0x%02X", static_cast<uint8_t>(message.frame.header.direction_node.direction));
  ESP_LOGD(tag, "    node_id: 0x%02X", message.frame.header.direction_node.node_id);
  
  // Delegate to the data struct's print_debug method
  message.data.standard.print_debug(tag, message.frame.header.command);
  
  ESP_LOGD(tag, "  Frame End:");
  ESP_LOGD(tag, "    crc: 0x%02X", message.frame_end.crc);
  ESP_LOGD(tag, "    prologue: 0x%02X", static_cast<uint8_t>(message.frame_end.prologue));
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
