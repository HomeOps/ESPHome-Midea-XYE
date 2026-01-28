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

float Preset::getCurrentInsideTemperatureForRealClimate() const {
  if (thermostat->inside_sensor_ != nullptr && thermostat->inside_sensor_->has_state()) {
    return thermostat->inside_sensor_->state;
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

optional<climate::ClimateMode> Preset::getModeForRealClimate() const {
  // If there's no real climate device, return empty optional
  if (thermostat->real_climate_ == nullptr) {
    return {};
  }
  
  if (id != climate::CLIMATE_PRESET_NONE) {
    const auto temp = getTargetTemperatureForRealClimate();
    const auto inside_temp = getCurrentInsideTemperatureForRealClimate();
    // Handle NaN case when inside sensor is unavailable
    if (std::isnan(inside_temp)) {
      // Don't change the real device mode when inside temperature is unavailable
      // Return current mode to keep device unchanged
      ESP_LOGD("virtual_thermostat", "Inside temperature unavailable, keeping current mode");
      return thermostat->real_climate_->mode;
    }
    
    // Check if inside temperature is outside the range
    if (inside_temp < min()) {
      return climate::CLIMATE_MODE_HEAT; // inside_temp too cold, need heating
    } else if (inside_temp > max()) {
      return climate::CLIMATE_MODE_COOL; // inside_temp too hot, need cooling
    } else {
      // Inside temperature is within range
      // Calculate midpoint once for use in both outside sensor logic and fallback
      const float mid_point = (min() + max()) / 2.0f;
      
      // Use outside temperature sensor to decide between cool or heat
      // This keeps the real thermostat ready in the appropriate mode
      if (thermostat->outside_sensor_ != nullptr && thermostat->outside_sensor_->has_state()) {
        const auto outside_temp = thermostat->outside_sensor_->state;
        if (!std::isnan(outside_temp)) {
          // Use outside temp to decide mode - if it's hot outside, stay ready to cool
          // if it's cold outside, stay ready to heat
          if (outside_temp < mid_point) {
            return climate::CLIMATE_MODE_HEAT; // cold outside, stay ready to heat
          } else {
            return climate::CLIMATE_MODE_COOL; // warm outside, stay ready to cool
          }
        }
      }
      
      // Fallback: If outside sensor unavailable, use inside temp position within range
      if (inside_temp < mid_point) {
        return climate::CLIMATE_MODE_HEAT; // closer to min, stay ready to heat
      } else {
        return climate::CLIMATE_MODE_COOL; // closer to max, stay ready to cool
      }
    }
  }
  else {
    // In manual mode, the real climate device is in control
    // Return its current mode to avoid changing it (don't send virtual mode which may be AUTO)
    return thermostat->real_climate_->mode;
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
