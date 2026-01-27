#pragma once

#include "esphome.h"

namespace esphome {
namespace virtual_thermostat {

// Forward declaration
class VirtualThermostat;

// Minimum temperature difference between min and max presets
constexpr float MIN_TEMP_DIFF = 1.0f;

struct Preset {
  Preset() = delete;
  Preset(climate::ClimatePreset id, VirtualThermostat *thermostat) : id(id), thermostat(thermostat) {}
  climate::ClimatePreset id;
  number::Number *min_entity_{nullptr};
  number::Number *max_entity_{nullptr};
  VirtualThermostat *thermostat{nullptr};
  bool updating_{false};  // Guard flag to prevent recursive updates
  // Note: min_entity() and max_entity() register callbacks that capture 'this'.
  // These Preset objects have the same lifetime as VirtualThermostat (member variables),
  // so the callbacks remain valid throughout the VirtualThermostat's lifetime.
  void min_entity(number::Number *n);
  void max_entity(number::Number *n);

  float min() const;
  float max() const;

  float getTargetTemperatureForRealClimate() const;

  float getCurrentRoomTemperatureForRealClimate() const;

  climate::ClimateFanMode getFanModeForRealClimate() const;

  climate::ClimateMode getModeForVirtualThermostat() const;

  climate::ClimateMode getModeForRealClimate() const;

  void on_min_changed(float new_min);
  void on_max_changed(float new_max);
};

}  // namespace virtual_thermostat
}  // namespace esphome