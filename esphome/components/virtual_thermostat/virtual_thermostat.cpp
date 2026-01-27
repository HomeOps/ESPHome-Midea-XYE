#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

// Minimum temperature difference between min and max presets
static const float MIN_TEMP_DIFF = 1.0f;

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
  return min_entity_ ? min_entity_->state : thermostat->target_temperature_low;
}

float Preset::max() const {
  return max_entity_ ? max_entity_->state : thermostat->target_temperature_high;
}

float Preset::getTargetTemperatureForRealClimate() const {
  return (min() + max()) / 2.0f;
}

float Preset::getCurrentRoomTemperatureForRealClimate() const {
  if (thermostat->room_sensor_ != nullptr) {
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
    const auto temp = getTargetTemperatureForRealClimate();
    const auto room_temp = getCurrentRoomTemperatureForRealClimate();
    // Handle NaN case when room sensor is unavailable
    if (std::isnan(room_temp)) {
      ESP_LOGD("virtual_thermostat", "Room temperature unavailable, defaulting to OFF mode");
      return climate::CLIMATE_MODE_OFF;
    }
    if (room_temp < min()) {
      return climate::CLIMATE_MODE_HEAT; // room_temp too cold, need heating
    } else if (room_temp > max()) {
      return climate::CLIMATE_MODE_COOL; // room_temp too hot, need cooling
    } else {
      return climate::CLIMATE_MODE_OFF; // within range, turn off
    }
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

VirtualThermostat::VirtualThermostat() {
}

void VirtualThermostat::setup() {
  // Restore previous state if available
  auto restored = this->restore_state_();

  if (restored.has_value()) {
      this->mode = restored->mode;
      this->fan_mode = restored->fan_mode;
      this->preset = restored->preset;
      this->target_temperature = restored->target_temperature;
      this->target_temperature_low = restored->target_temperature_low;
      this->target_temperature_high = restored->target_temperature_high;
  }
  const auto& active_preset = getActivePreset();
  apply_preset(active_preset);
  this->publish_state();
}

climate::ClimateTraits VirtualThermostat::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supported_modes({climate::CLIMATE_MODE_AUTO, climate::CLIMATE_MODE_HEAT, climate::CLIMATE_MODE_COOL});
  traits.add_feature_flags(
    climate::ClimateFeature::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | // show current temperature
    climate::ClimateFeature::CLIMATE_SUPPORTS_TWO_POINT_TARGET_TEMPERATURE | // AUTO mode uses min/max
    climate::ClimateFeature::CLIMATE_SUPPORTS_ACTION // show heating/cooling/idle
  );
  traits.set_supported_fan_modes({
    climate::ClimateFanMode::CLIMATE_FAN_AUTO,
    climate::ClimateFanMode::CLIMATE_FAN_LOW,
    climate::ClimateFanMode::CLIMATE_FAN_MEDIUM,
    climate::ClimateFanMode::CLIMATE_FAN_HIGH
  });
  traits.set_supported_presets({home.id, sleep.id, away.id, manual.id});
  return traits;
}

void VirtualThermostat::apply_preset(const Preset& p) {
  this->preset = p.id;
  const float temp = p.getTargetTemperatureForRealClimate();
  if (p.id != manual.id) {
    this->target_temperature_low  = p.min();
    this->target_temperature_high = p.max();
    this->real_climate_->target_temperature = temp;
  } else {
    this->target_temperature = temp;
    this->real_climate_->target_temperature = temp;
  }
  this->mode = p.getModeForVirtualThermostat();
  this->real_climate_->mode = p.getModeForRealClimate();
  this->real_climate_->fan_mode = p.getFanModeForRealClimate();
  
  this->real_climate_->publish_state();
  this->publish_state();
}

const Preset& VirtualThermostat::getActivePreset() const {
  const auto id = this->preset;
  if (id == home.id)  return home;
  if (id == sleep.id) return sleep;
  if (id == away.id)  return away;
  return manual;  // unknown or empty → manual
}

const Preset& VirtualThermostat::getActivePresetFromId(climate::ClimatePreset id) const {
  if (id == home.id)  return home;
  if (id == sleep.id) return sleep;
  if (id == away.id)  return away;
  return manual;  // unknown or empty → manual
}

void VirtualThermostat::exit_preset_mode() {
  apply_preset(manual);
}

void VirtualThermostat::control(const climate::ClimateCall &call) {

  if (call.get_fan_mode().has_value()) {
    this->fan_mode = *call.get_fan_mode();
    this->real_climate_->fan_mode = *call.get_fan_mode();
    this->real_climate_->publish_state();
    this->publish_state();
  }

  // PRESET CHANGE
  if (call.get_preset().has_value()) {
    const auto preset_id = *call.get_preset();
    const auto& active_preset = getActivePresetFromId(preset_id);
    apply_preset(active_preset);
  }

  // MANUAL EDITS → EXIT PRESET MODE
  if (call.get_target_temperature_low().has_value()) {
    this->target_temperature_low = *call.get_target_temperature_low();
    apply_preset(manual);
  }

  if (call.get_target_temperature_high().has_value()) {
    this->target_temperature_high = *call.get_target_temperature_high();
    apply_preset(manual);
  }

  // MODE CHANGE
  if (call.get_mode().has_value()) {
    const auto new_mode = *call.get_mode();
    const auto& active_preset = getActivePreset();

    // AUTO → HEAT/COOL
    if ((new_mode == climate::CLIMATE_MODE_HEAT ||
         new_mode == climate::CLIMATE_MODE_COOL) &&
        this->mode == climate::CLIMATE_MODE_AUTO) {
      const float temp = active_preset.getTargetTemperatureForRealClimate();
      this->target_temperature = temp;
    }

    // HEAT/COOL → AUTO (only re-apply preset if in a real preset, not manual)
    if (new_mode == climate::CLIMATE_MODE_AUTO &&
        (this->mode == climate::CLIMATE_MODE_HEAT ||
         this->mode == climate::CLIMATE_MODE_COOL) &&
        active_preset.id != manual.id) {
      apply_preset(active_preset);
      return; // apply_preset already publishes state
    }

    // Update mode and sync to real climate
    this->mode = new_mode;
    this->real_climate_->mode = active_preset.getModeForRealClimate();
    this->real_climate_->target_temperature = active_preset.getTargetTemperatureForRealClimate();
    this->real_climate_->publish_state();
    this->publish_state();
    return; // Already published state
  }

  if (call.get_target_temperature().has_value()) {
    const float temp = *call.get_target_temperature();
    this->target_temperature = temp;
    this->real_climate_->target_temperature = temp;
    this->real_climate_->publish_state();
    exit_preset_mode();
    return; // exit_preset_mode already publishes state
  }

  this->publish_state();
}

void VirtualThermostat::loop() {
  // Periodic update to synchronize real climate with virtual state
  uint32_t now = millis();
  if (now - this->last_update_time_ >= this->update_interval_ms_) {
    this->last_update_time_ = now;
    update_real_climate();
  }
  
  // Update current temperature from room sensor
  if (this->room_sensor_ != nullptr && !std::isnan(this->room_sensor_->state)) {
    this->current_temperature = this->room_sensor_->state;
  }
}

void VirtualThermostat::update_real_climate() {
  if (!this->real_climate_) return;
  
  const auto& active_preset = getActivePreset();
  
  // Update real climate based on current virtual state
  this->real_climate_->mode = active_preset.getModeForRealClimate();
  this->real_climate_->fan_mode = active_preset.getFanModeForRealClimate();
  this->real_climate_->target_temperature = active_preset.getTargetTemperatureForRealClimate();
  
  this->real_climate_->publish_state();
}

}  // namespace virtual_thermostat
}  // namespace esphome

