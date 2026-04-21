#ifdef USE_ARDUINO

#include "xye_adapter.h"

namespace esphome {
namespace midea {
namespace xye {

using climate::ClimateAction;
using climate::ClimateFanMode;
using climate::ClimateMode;

climate::ClimateMode XYEAdapter::get_climate_mode(OperationMode op_mode) noexcept {
  ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
  switch (static_cast<uint8_t>(op_mode) & OP_MODE_VALUE_MASK) {
    case static_cast<uint8_t>(OperationMode::OFF):  mode = ClimateMode::CLIMATE_MODE_OFF;       break;
    case static_cast<uint8_t>(OperationMode::AUTO): mode = ClimateMode::CLIMATE_MODE_HEAT_COOL; break;
    case static_cast<uint8_t>(OperationMode::FAN):  mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;  break;
    case static_cast<uint8_t>(OperationMode::DRY):  mode = ClimateMode::CLIMATE_MODE_DRY;       break;
    case static_cast<uint8_t>(OperationMode::HEAT): mode = ClimateMode::CLIMATE_MODE_HEAT;      break;
    case static_cast<uint8_t>(OperationMode::COOL): mode = ClimateMode::CLIMATE_MODE_COOL;      break;
  }
  // AUTO_FLAG (0x10) overrides the base value: if set while not already OFF,
  // the unit is in heat/cool (auto) mode.
  if (mode != ClimateMode::CLIMATE_MODE_OFF &&
      (static_cast<uint8_t>(op_mode) & OP_MODE_AUTO_FLAG) == OP_MODE_AUTO_FLAG) {
    mode = ClimateMode::CLIMATE_MODE_HEAT_COOL;
  }
  return mode;
}

climate::ClimateFanMode XYEAdapter::get_climate_fan_mode(FanMode fan_mode) noexcept {
  // The AUTO flag (bit 7, 0x80) takes precedence over the speed nibble.
  if ((static_cast<uint8_t>(fan_mode) & static_cast<uint8_t>(FanMode::FAN_AUTO)) ==
      static_cast<uint8_t>(FanMode::FAN_AUTO))
    return ClimateFanMode::CLIMATE_FAN_AUTO;
  switch (static_cast<uint8_t>(fan_mode) & FAN_SPEED_MASK) {
    case static_cast<uint8_t>(FanMode::FAN_HIGH):   return ClimateFanMode::CLIMATE_FAN_HIGH;
    case static_cast<uint8_t>(FanMode::FAN_MEDIUM): return ClimateFanMode::CLIMATE_FAN_MEDIUM;
    case static_cast<uint8_t>(FanMode::FAN_LOW):    return ClimateFanMode::CLIMATE_FAN_LOW;
    case static_cast<uint8_t>(FanMode::FAN_OFF):    return ClimateFanMode::CLIMATE_FAN_OFF;
    default:                                         return ClimateFanMode::CLIMATE_FAN_AUTO;
  }
}

float XYEAdapter::get_temperature(uint8_t raw) noexcept { return Temperature{raw}.to_celsius(); }

float XYEAdapter::get_target_temperature(uint8_t raw) noexcept {
  return static_cast<float>(raw & SET_TEMP_VALUE_MASK);
}

climate::ClimateAction XYEAdapter::get_climate_action(climate::ClimateMode mode, FanMode fan_mode,
                                                       OperationMode op_mode) noexcept {
  const bool fan_running = (static_cast<uint8_t>(fan_mode) & FAN_SPEED_MASK) != 0x00;

  ClimateAction action = ClimateAction::CLIMATE_ACTION_IDLE;
  if (mode == ClimateMode::CLIMATE_MODE_HEAT && fan_running) {
    action = ClimateAction::CLIMATE_ACTION_HEATING;
  } else if (mode == ClimateMode::CLIMATE_MODE_COOL && fan_running) {
    action = ClimateAction::CLIMATE_ACTION_COOLING;
  }

  // In heat/cool (auto) mode refine the action using the unit's reported sub-mode.
  if (mode == ClimateMode::CLIMATE_MODE_HEAT_COOL) {
    const uint8_t op_raw = static_cast<uint8_t>(op_mode) & OP_MODE_VALUE_MASK;
    if (op_raw == static_cast<uint8_t>(OperationMode::COOL))
      action = ClimateAction::CLIMATE_ACTION_COOLING;
    else if (op_raw == static_cast<uint8_t>(OperationMode::FAN))
      action = ClimateAction::CLIMATE_ACTION_FAN;
    else if (op_raw == static_cast<uint8_t>(OperationMode::HEAT))
      action = ClimateAction::CLIMATE_ACTION_HEATING;
  }

  return action;
}

OperationMode XYEAdapter::get_operation_mode(climate::ClimateMode mode) noexcept {
  switch (mode) {
    case ClimateMode::CLIMATE_MODE_HEAT_COOL: return OperationMode::AUTO;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:  return OperationMode::FAN;
    case ClimateMode::CLIMATE_MODE_DRY:       return OperationMode::DRY;
    case ClimateMode::CLIMATE_MODE_HEAT:      return OperationMode::HEAT;
    case ClimateMode::CLIMATE_MODE_COOL:      return OperationMode::COOL;
    default:                                  return OperationMode::OFF;
  }
}

FanMode XYEAdapter::get_fan_mode(climate::ClimateFanMode fan_mode) noexcept {
  switch (fan_mode) {
    case ClimateFanMode::CLIMATE_FAN_HIGH:   return FanMode::FAN_HIGH;
    case ClimateFanMode::CLIMATE_FAN_MEDIUM: return FanMode::FAN_MEDIUM;
    case ClimateFanMode::CLIMATE_FAN_LOW:    return FanMode::FAN_LOW;
    case ClimateFanMode::CLIMATE_FAN_OFF:    return FanMode::FAN_OFF;
    default:                                 return FanMode::FAN_AUTO;
  }
}

uint8_t XYEAdapter::get_raw_target_temperature(float celsius, bool use_fahrenheit) noexcept {
  if (use_fahrenheit) {
    float fahrenheit = (9.0f / 5.0f) * celsius + 32.0f;
    return static_cast<uint8_t>(static_cast<int>(fahrenheit) + FAHRENHEIT_TEMP_OFFSET);
  }
  return static_cast<uint8_t>(static_cast<int>(celsius));
}

ModeFlags XYEAdapter::get_mode_flags(climate::ClimatePreset preset,
                                     climate::ClimateSwingMode swing_mode) noexcept {
  return static_cast<ModeFlags>(
      ((preset == climate::ClimatePreset::CLIMATE_PRESET_BOOST) * static_cast<uint8_t>(ModeFlags::AUX_HEAT)) |
      ((preset == climate::ClimatePreset::CLIMATE_PRESET_SLEEP) * static_cast<uint8_t>(ModeFlags::ECO)) |
      ((swing_mode != climate::ClimateSwingMode::CLIMATE_SWING_OFF) * static_cast<uint8_t>(ModeFlags::SWING)));
}

bool XYEAdapter::is_defrost_active(uint16_t protect_flags) noexcept {
  return (protect_flags & DEFROST_PROTECT_FLAG) != 0;
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
