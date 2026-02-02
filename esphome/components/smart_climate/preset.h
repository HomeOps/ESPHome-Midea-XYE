#pragma once

#include "esphome.h"

namespace esphome {
namespace smart_climate {

// Forward declaration
class SmartClimate;

// Minimum temperature difference between min and max presets
constexpr float MIN_TEMP_DIFF = 1.0f;

struct Preset {
  Preset() = delete;
  Preset(climate::ClimatePreset id, SmartClimate *thermostat) : id(id), thermostat(thermostat) {}
  climate::ClimatePreset id;
  
  // IMPORTANT: Callback lifetime safety
  // min_entity() and max_entity() register callbacks that capture 'this' pointer.
  // These Preset objects are member variables of SmartClimate (not dynamically allocated),
  // ensuring they have the same lifetime as the SmartClimate instance.
  // The number entities are also owned by ESPHome's component system and will not outlive
  // the SmartClimate, making these callbacks safe from dangling pointer issues.
  void min_entity(number::Number *n);
  void max_entity(number::Number *n);

  float min() const;
  float max() const;

  float getTargetTemperatureForRealClimate() const;

  float getCurrentInsideTemperatureForRealClimate() const;

  climate::ClimateFanMode getFanModeForRealClimate() const;

  climate::ClimateMode getModeForSmartClimate() const;

  optional<climate::ClimateMode> getModeForRealClimate() const;

private:
  number::Number *min_entity_{nullptr};
  number::Number *max_entity_{nullptr};
  SmartClimate *thermostat{nullptr};
  bool updating_{false};  // Guard flag to prevent recursive updates
  
  void on_min_changed(float new_min);
  void on_max_changed(float new_max);
};

}  // namespace smart_climate
}  // namespace esphome