#pragma once

#ifdef USE_ARDUINO

#include "xye.h"
#include "esphome/components/climate/climate.h"

namespace esphome {
namespace midea {
namespace xye {

/// @brief Monotonic fan-speed level produced by the adapter for the `fan_speed` sensor.
///
/// This is an adapter output type, not an XYE protocol value — the raw protocol
/// nibble (FanMode) is not ordered by speed (FAN_HIGH is 0x01 while FAN_LOW is
/// 0x03/0x04). XYEAdapter::get_fan_speed_level remaps it onto this ordinal scale
/// so the published sensor value sorts and graphs correctly. It deliberately
/// lives here, beside the adapter, rather than in the protocol header xye.h.
///
/// Enumerators carry a SPEED_ prefix because the Arduino framework defines LOW
/// and HIGH as preprocessor macros, which would otherwise mangle bare names.
enum class FanSpeedLevel : uint8_t {
  SPEED_OFF = 0,     ///< Fan not running
  SPEED_LOW = 1,     ///< Low speed
  SPEED_MEDIUM = 2,  ///< Medium speed
  SPEED_HIGH = 3     ///< High speed
};

/// @brief Static adapter class that bridges XYE protocol values and ESPHome climate entity types.
///        All methods are static and noexcept, performing pure value conversions with no side effects.
struct XYEAdapter {
  /// Returns the ESPHome ClimateMode for the given XYE OperationMode byte.
  static climate::ClimateMode get_climate_mode(OperationMode op_mode) noexcept;

  /// Returns the ESPHome ClimateFanMode for the given XYE FanMode byte.
  static climate::ClimateFanMode get_climate_fan_mode(FanMode fan_mode) noexcept;

  /// Returns the ESPHome ClimateFanMode for the given XYE TargetFanSpeed value.
  /// TargetFanSpeed is the commanded speed from the physical thermostat (C4 packet),
  /// which persists even when the fan is idle — unlike FanMode in C0 which reads 0x00 when stopped.
  static climate::ClimateFanMode get_climate_fan_mode(TargetFanSpeed target) noexcept;

  /// Returns the monotonic FanSpeedLevel ordinal for the given XYE FanMode byte.
  /// The raw protocol nibble is not ordered by speed; this remaps it for the fan_speed sensor.
  static FanSpeedLevel get_fan_speed_level(FanMode fan_mode) noexcept;

  /// Returns the decoded Celsius temperature from an encoded XYE temperature byte.
  /// Used for T1/T2/T3/outdoor temperature readings and the C4 setpoint when not in Fahrenheit.
  static float get_temperature(uint8_t raw) noexcept;

  /// Returns the target setpoint in Celsius from a raw XYE C4 temperature byte.
  /// @param raw  The raw byte from the C4 `target_temperature` field.
  /// @param use_fahrenheit  When true the byte encodes a Fahrenheit value:
  ///        celsius = (raw - FAHRENHEIT_TEMP_OFFSET - 32) * 5/9.
  ///        When false the byte is a Celsius integer with SET_TEMP_STATUS_FLAG
  ///        (bit 6, 0x40) masked off before conversion.
  static float get_target_temperature(uint8_t raw, bool use_fahrenheit = false) noexcept;

  /// Returns the ClimateAction derived from the current mode, fan, operation state,
  /// compressor status, and defrost state.
  /// @param compressor_active true when the compressor is actively running (C0 byte [19]).
  ///        HEATING/COOLING is reported only when this is set; otherwise the fan is just
  ///        circulating air (e.g. a compressor-protection delay between cycles) and FAN is
  ///        reported instead.
  /// @param defrost_active true when the unit is running a defrost cycle. A heating action
  ///        is downgraded to IDLE because the reversed refrigeration cycle is melting
  ///        outdoor-coil ice, not warming the room.
  /// @note Intended for use when mode != CLIMATE_MODE_OFF.
  static climate::ClimateAction get_climate_action(climate::ClimateMode mode, FanMode fan_mode,
                                                   OperationMode op_mode, bool compressor_active,
                                                   bool defrost_active) noexcept;

  /// Returns the XYE OperationMode for the given ESPHome ClimateMode.
  static OperationMode get_operation_mode(climate::ClimateMode mode) noexcept;

  /// Returns the XYE FanMode for the given ESPHome ClimateFanMode.
  static FanMode get_fan_mode(climate::ClimateFanMode fan_mode) noexcept;

  /// Returns the raw XYE target-temperature byte for the given ESPHome setpoint.
  /// When use_fahrenheit is true the Celsius value is converted to Fahrenheit before encoding.
  static uint8_t get_raw_target_temperature(float celsius, bool use_fahrenheit) noexcept;

  /// Returns the XYE ModeFlags for the given ESPHome preset and swing mode.
  static ModeFlags get_mode_flags(climate::ClimatePreset preset,
                                  climate::ClimateSwingMode swing_mode) noexcept;

  /// Returns true when the defrost bit is set in the C0 QUERY response `protect_flags` value.
  static bool is_defrost_active(uint16_t protect_flags) noexcept;
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
