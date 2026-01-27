#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

void Preset::min_entity(number::Number *n) { min_entity_ = n; }
void Preset::max_entity(number::Number *n) { max_entity_ = n; }

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
    return getModeForRealClimate(); // passthrough to the real climate
  }
}

climate::ClimateMode Preset::getModeForRealClimate() const {
  if (id != climate::CLIMATE_PRESET_NONE) {
    const auto temp = getTargetTemperatureForRealClimate();
    const auto room_temp = getCurrentRoomTemperatureForRealClimate();
    if (room_temp < min()) {
      return climate::CLIMATE_MODE_HEAT; // room_temp too cold, need heating
    } else if (room_temp > max()) {
      return climate::CLIMATE_MODE_COOL; // root_temp too hot, need cooling
    } else {
      return climate::CLIMATE_MODE_OFF; // within range, turn off
    }
  }
  else {
    return thermostat->real_climate_->mode;
  }
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
    this->publish_state(); // Do we need to publish state here?
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
    const float temp = active_preset.getTargetTemperatureForRealClimate();

    // AUTO → HEAT/COOL
    if ((new_mode == climate::CLIMATE_MODE_HEAT ||
         new_mode == climate::CLIMATE_MODE_COOL) &&
        this->mode == climate::CLIMATE_MODE_AUTO) {
      this->target_temperature = temp;
      this->real_climate_->target_temperature = active_preset.getTargetTemperatureForRealClimate();
      this->real_climate_->publish_state();
      this->publish_state();
    }

    // HEAT/COOL → AUTO
    if (new_mode == climate::CLIMATE_MODE_AUTO &&
        (this->mode == climate::CLIMATE_MODE_HEAT ||
         this->mode == climate::CLIMATE_MODE_COOL)) {
      apply_preset(active_preset);
    }

    this->mode = new_mode;
  }

  if (call.get_target_temperature().has_value()) {
    const float temp = *call.get_target_temperature();
    this->target_temperature = temp;
    this->real_climate_->target_temperature = temp;
    this->real_climate_->publish_state();
    exit_preset_mode();
  }

  this->publish_state();
}

void VirtualThermostat::calculateRealClimateState() {

  if (!this->room_sensor_ || !this->real_climate_) return;

  const float room = this->room_sensor_->state;

  // AUTO MODE
  if (this->mode == climate::CLIMATE_MODE_AUTO) {
    const auto minv = this->target_temperature_low;
    const auto maxv = this->target_temperature_high;

    if (room < minv) {
      this->action = climate::CLIMATE_ACTION_HEATING;
      this->real_climate_->mode = climate::CLIMATE_MODE_HEAT;
      this->real_climate_->target_temperature = minv;
    }
    else if (room > maxv) {
      this->action = climate::CLIMATE_ACTION_COOLING;
      this->real_climate_->mode = climate::CLIMATE_MODE_COOL;
      this->real_climate_->target_temperature = maxv;
    }
    else {
      this->action = climate::CLIMATE_ACTION_IDLE;
      // Do NOT turn off the real AC
    }
  }

  // HEAT MODE
  if (this->mode == climate::CLIMATE_MODE_HEAT) {
    const auto t = this->target_temperature;
    this->action = climate::CLIMATE_ACTION_HEATING;
    this->real_climate_->mode = climate::CLIMATE_MODE_HEAT;
    this->real_climate_->target_temperature = t;
  }

  // COOL MODE
  if (this->mode == climate::CLIMATE_MODE_COOL) {
    const auto t = this->target_temperature;
    this->action = climate::CLIMATE_ACTION_COOLING;
    this->real_climate_->mode = climate::CLIMATE_MODE_COOL;
    this->real_climate_->target_temperature = t;
  }

  // FAN MODE PASSTHROUGH
  if (this->fan_mode == climate::CLIMATE_FAN_AUTO)
    this->real_climate_->fan_mode = climate::CLIMATE_FAN_AUTO;
  else if (this->fan_mode == climate::CLIMATE_FAN_LOW)
    this->real_climate_->fan_mode = climate::CLIMATE_FAN_LOW;
  else if (this->fan_mode == climate::CLIMATE_FAN_MEDIUM)
    this->real_climate_->fan_mode = climate::CLIMATE_FAN_MEDIUM;
  else if (this->fan_mode == climate::CLIMATE_FAN_HIGH)
    this->real_climate_->fan_mode = climate::CLIMATE_FAN_HIGH;

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

