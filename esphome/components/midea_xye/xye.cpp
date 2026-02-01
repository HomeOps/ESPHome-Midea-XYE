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

size_t Temperature::print_debug(const char *tag, const char *name, size_t left, int level) const {
  if (left < sizeof(Temperature)) return left;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%02X (%.1f°C)"), 
           name, value, to_celsius());
  return left - sizeof(Temperature);
}

// Flags16 methods
uint16_t Flags16::value() const {
  return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

void Flags16::set(uint16_t val) {
  low = val & 0xFF;
  high = (val >> 8) & 0xFF;
}

size_t Flags16::print_debug(const char *tag, const char *name, size_t left, int level) const {
  if (left < sizeof(Flags16)) return left;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%04X"), 
           name, value());
  return left - sizeof(Flags16);
}

// Flags16BigEndian methods
uint16_t Flags16BigEndian::value() const {
  return static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8);
}

void Flags16BigEndian::set(uint16_t val) {
  low = val & 0xFF;
  high = (val >> 8) & 0xFF;
}

size_t Flags16BigEndian::print_debug(const char *tag, const char *name, size_t left, int level) const {
  if (left < sizeof(Flags16BigEndian)) return left;
  uint16_t val = value();
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: 0x%04X (%u)"), 
           name, val, val);
  return left - sizeof(Flags16BigEndian);
}

// DirectionNode methods
size_t DirectionNode::print_debug(const char *tag, const char *name, size_t left, int level) const {
  if (left < sizeof(DirectionNode)) return left;
  ::esphome::esp_log_printf_(level, tag, __LINE__, ESPHOME_LOG_FORMAT("    %s: direction=0x%02X (%s), node_id=0x%02X"), 
           name, static_cast<uint8_t>(direction), enum_to_string(direction), node_id);
  return left - sizeof(DirectionNode);
}

// Static maps for enum-to-string conversion
const std::map<Command, const char*> COMMAND_MAP = {
  {Command::QUERY, "QUERY"},
  {Command::QUERY_EXTENDED, "QUERY_EXTENDED"},
  {Command::SET, "SET"},
  {Command::FOLLOW_ME, "FOLLOW_ME"},
  {Command::LOCK, "LOCK"},
  {Command::UNLOCK, "UNLOCK"},
};

const std::map<OperationMode, const char*> OPERATION_MODE_MAP = {
  {OperationMode::OFF, "OFF"},
  {OperationMode::AUTO, "AUTO"},
  {OperationMode::FAN, "FAN"},
  {OperationMode::DRY, "DRY"},
  {OperationMode::HEAT, "HEAT"},
  {OperationMode::COOL, "COOL"},
  {OperationMode::AUTO_ALT, "AUTO_ALT"},
};

const std::map<FanMode, const char*> FAN_MODE_MAP = {
  {FanMode::FAN_OFF, "FAN_OFF"},
  {FanMode::FAN_HIGH, "FAN_HIGH"},
  {FanMode::FAN_MEDIUM, "FAN_MEDIUM"},
  {FanMode::FAN_LOW_ALT, "FAN_LOW_ALT"},
  {FanMode::FAN_LOW, "FAN_LOW"},
  {FanMode::FAN_AUTO, "FAN_AUTO"},
};

const std::map<ModeFlags, const char*> MODE_FLAGS_MAP = {
  {ModeFlags::NORMAL, "NORMAL"},
  {ModeFlags::ECO, "ECO"},
  {ModeFlags::AUX_HEAT, "AUX_HEAT"},
  {ModeFlags::SWING, "SWING"},
  {ModeFlags::VENTILATION, "VENTILATION"},
};

const std::map<OperationFlags, const char*> OPERATION_FLAGS_MAP = {
  {OperationFlags::WATER_PUMP, "WATER_PUMP"},
  {OperationFlags::WATER_LOCK, "WATER_LOCK"},
};

const std::map<Capabilities, const char*> CAPABILITIES_MAP = {
  {Capabilities::EXTERNAL_TEMP, "EXTERNAL_TEMP"},
  {Capabilities::SWING, "SWING"},
};

const std::map<Direction, const char*> DIRECTION_MAP = {
  {Direction::FROM_CLIENT, "FROM_CLIENT"},
  {Direction::TO_CLIENT, "TO_CLIENT"},
};

const std::map<CcmErrorFlags, const char*> CCM_ERROR_FLAGS_MAP = {
  {CcmErrorFlags::NO_ERROR, "NO_ERROR"},
  {CcmErrorFlags::TIMEOUT, "TIMEOUT"},
  {CcmErrorFlags::CRC_ERROR, "CRC_ERROR"},
  {CcmErrorFlags::PROTOCOL_ERROR, "PROTOCOL_ERROR"},
};

const std::map<FollowMeSubcommand, const char*> FOLLOW_ME_SUBCOMMAND_MAP = {
  {FollowMeSubcommand::UPDATE, "UPDATE"},
  {FollowMeSubcommand::STATIC_PRESSURE, "STATIC_PRESSURE"},
  {FollowMeSubcommand::INIT, "INIT"},
};

const std::map<CompressorFlags, const char*> COMPRESSOR_FLAGS_MAP = {
  {CompressorFlags::IDLE, "IDLE"},
  {CompressorFlags::ACTIVE, "ACTIVE"},
};

const std::map<EspProfile, const char*> ESP_PROFILE_MAP = {
  {EspProfile::ESP_LOW, "ESP_LOW"},
  {EspProfile::ESP_MEDIUM, "ESP_MEDIUM"},
  {EspProfile::ESP_HIGH, "ESP_HIGH"},
};

const std::map<ProtectionFlags, const char*> PROTECTION_FLAGS_MAP = {
  {ProtectionFlags::NONE, "NONE"},
  {ProtectionFlags::OUTDOOR_FAN_RUNNING, "OUTDOOR_FAN_RUNNING"},
  {ProtectionFlags::COMPRESSOR_ACTIVE, "COMPRESSOR_ACTIVE"},
};

const std::map<SystemStatusFlags, const char*> SYSTEM_STATUS_FLAGS_MAP = {
  {SystemStatusFlags::DISABLED, "DISABLED"},
  {SystemStatusFlags::WIRED_CONTROLLER, "WIRED_CONTROLLER"},
  {SystemStatusFlags::ENABLED, "ENABLED"},
  {SystemStatusFlags::ENABLED_WITH_CONTROLLER, "ENABLED_WITH_CONTROLLER"},
};

const std::map<SubsystemFlags, const char*> SUBSYSTEM_FLAGS_MAP = {
  {SubsystemFlags::PROTECTION_ACTIVE, "PROTECTION_ACTIVE"},
  {SubsystemFlags::OK, "OK"},
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
