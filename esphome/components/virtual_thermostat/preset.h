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
  
  // IMPORTANT: Callback lifetime safety
  // min_entity() and max_entity() register callbacks that capture 'this' pointer.
  // These Preset objects are member variables of VirtualThermostat (not dynamically allocated),
  // ensuring they have the same lifetime as the VirtualThermostat instance.
  // The number entities are also owned by ESPHome's component system and will not outlive
  // the VirtualThermostat, making these callbacks safe from dangling pointer issues.
  void min_entity(number::Number *n);
  void max_entity(number::Number *n);

  float min() const;
  float max() const;

  float getTargetTemperatureForRealClimate() const;

  float getCurrentRoomTemperatureForRealClimate() const;

  climate::ClimateFanMode getFanModeForRealClimate() const;

  climate::ClimateMode getModeForVirtualThermostat() const;

  optional<climate::ClimateMode> getModeForRealClimate() const;

private:
  number::Number *min_entity_{nullptr};
  number::Number *max_entity_{nullptr};
  VirtualThermostat *thermostat{nullptr};
  bool updating_{false};  // Guard flag to prevent recursive updates
  
  void on_min_changed(float new_min);
  void on_max_changed(float new_max);
};

}  // namespace virtual_thermostat
}  // namespace esphome