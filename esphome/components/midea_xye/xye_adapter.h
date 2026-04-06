#pragma once

#ifdef USE_ARDUINO

#include "xye.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace midea {
namespace xye {

/// @brief Static adapter class that bridges XYE protocol values and ESPHome climate entity types.
///        All methods are static and noexcept, performing pure value conversions with no side effects.
struct XYEAdapter {
  /// Returns the ESPHome ClimateMode for the given XYE OperationMode byte.
  static climate::ClimateMode get_climate_mode(OperationMode op_mode) noexcept;

  /// Returns the ESPHome ClimateFanMode for the given XYE FanMode byte.
  static climate::ClimateFanMode get_climate_fan_mode(FanMode fan_mode) noexcept;

  /// Returns the decoded Celsius temperature from a raw XYE temperature byte.
  static float get_temperature(uint8_t raw) noexcept;

  /// Returns the target temperature in Celsius from a raw XYE byte,
  /// masking out the SET_TEMP_STATUS_FLAG (bit 6) that the unit may set in certain states.
  static float get_target_temperature(uint8_t raw) noexcept;

  /// Returns the ClimateAction derived from the current mode, fan, and operation state.
  /// @note Intended for use when mode != CLIMATE_MODE_OFF.
  static climate::ClimateAction get_climate_action(climate::ClimateMode mode, FanMode fan_mode,
                                                   OperationMode op_mode) noexcept;

  /// Returns the XYE OperationMode for the given ESPHome ClimateMode.
  static OperationMode get_operation_mode(climate::ClimateMode mode) noexcept;

  /// Returns the XYE FanMode for the given ESPHome ClimateFanMode.
  static FanMode get_fan_mode(climate::ClimateFanMode fan_mode) noexcept;
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
