#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

void Preset::min_entity(number::Number *n) {
  min_entity_ = n;
  if (n) {
    n->add_on_state_callback([this](float new_min) { on_min_changed(new_min); });
  }
}

void Preset::max_entity(number::Number *n) {
  max_entity_ = n;
  if (n) {
    n->add_on_state_callback([this](float new_max) { on_max_changed(new_max); });
  }
}

float Preset::min() const {
  return (min_entity_ && min_entity_->has_state()) ? min_entity_->state : thermostat->target_temperature_low;
}

float Preset::max() const {
  return (max_entity_ && max_entity_->has_state()) ? max_entity_->state : thermostat->target_temperature_high;
}

float Preset::getTargetTemperatureForRealClimate() const {
  return (min() + max()) / 2.0f;
}

float Preset::getCurrentRoomTemperatureForRealClimate() const {
  if (thermostat->room_sensor_ != nullptr && thermostat->room_sensor_->has_state()) {
    return thermostat->room_sensor_->state;
  }
  return NAN;
}

climate::ClimateFanMode Preset::getFanModeForRealClimate() const {
  return thermostat->fan_mode.value_or(climate::CLIMATE_FAN_AUTO);
}

climate::ClimateMode Preset::getModeForVirtualThermostat() const {
  if (id != climate::CLIMATE_PRESET_NONE) {
    return climate::CLIMATE_MODE_AUTO;
  } else {
    // In manual mode, keep the current mode of the virtual thermostat
    // This should only be called during initial setup; normally we don't change the mode when switching to manual
    return thermostat->mode;
  }
}

climate::ClimateMode Preset::getModeForRealClimate() const {
  if (id != climate::CLIMATE_PRESET_NONE) {
    // When in a preset mode (home, sleep, away), keep the real device in AUTO mode
    // so it never turns off but intelligently switches between heating and cooling
    // based on its target temperature. This ensures the HVAC stays ready.
    return climate::CLIMATE_MODE_AUTO;
  }
  else {
    // In manual mode, use the virtual thermostat's mode directly
    return thermostat->mode;
  }
}

void Preset::on_min_changed(float new_min) {
  if (updating_ || !max_entity_) return;
  
  // Set guard to prevent recursive updates
  updating_ = true;
  
  // If new min is greater than or equal to current max, update max to be greater than min
  if (max_entity_->has_state() && new_min >= max_entity_->state) {
    float new_max = new_min + MIN_TEMP_DIFF;
    // Ensure new_max is within the number entity's configured range
    if (max_entity_->traits.get_max_value() >= new_max) {
      max_entity_->publish_state(new_max);
    } else {
      ESP_LOGW("virtual_thermostat", "Cannot adjust max temperature to %.1f (exceeds maximum %.1f)",
               new_max, max_entity_->traits.get_max_value());
      // Revert min to maintain valid state where min < max
      if (min_entity_ && max_entity_->has_state()) {
        float safe_min = max_entity_->state - MIN_TEMP_DIFF;
        if (min_entity_->traits.get_min_value() <= safe_min) {
          min_entity_->publish_state(safe_min);
        }
      }
    }
  }
  
  // If this is the active preset, update the thermostat's target temperatures
  if (thermostat->preset == id && thermostat->mode == climate::CLIMATE_MODE_AUTO) {
    thermostat->target_temperature_low = new_min;
    if (new_min >= thermostat->target_temperature_high) {
      thermostat->target_temperature_high = new_min + MIN_TEMP_DIFF;
    }
    thermostat->publish_state();
  }
  
  updating_ = false;
}

void Preset::on_max_changed(float new_max) {
  if (updating_ || !min_entity_) return;
  
  // Set guard to prevent recursive updates
  updating_ = true;
  
  // If new max is less than or equal to current min, update min to be less than max
  if (min_entity_->has_state() && new_max <= min_entity_->state) {
    float new_min = new_max - MIN_TEMP_DIFF;
    // Ensure new_min is within the number entity's configured range
    if (min_entity_->traits.get_min_value() <= new_min) {
      min_entity_->publish_state(new_min);
    } else {
      ESP_LOGW("virtual_thermostat", "Cannot adjust min temperature to %.1f (below minimum %.1f)",
               new_min, min_entity_->traits.get_min_value());
      // Revert max to maintain valid state where max > min
      if (max_entity_ && min_entity_->has_state()) {
        float safe_max = min_entity_->state + MIN_TEMP_DIFF;
        if (max_entity_->traits.get_max_value() >= safe_max) {
          max_entity_->publish_state(safe_max);
        }
      }
    }
  }
  
  // If this is the active preset, update the thermostat's target temperatures
  if (thermostat->preset == id && thermostat->mode == climate::CLIMATE_MODE_AUTO) {
    thermostat->target_temperature_high = new_max;
    if (new_max <= thermostat->target_temperature_low) {
      thermostat->target_temperature_low = new_max - MIN_TEMP_DIFF;
    }
    thermostat->publish_state();
  }
  
  updating_ = false;
}

}  // namespace virtual_thermostat
}  // namespace esphome
