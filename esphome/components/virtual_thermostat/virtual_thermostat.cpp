#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

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

std::pair<bool, bool> VirtualThermostat::apply_preset(const Preset& p) {
  bool virtual_changed = false;
  bool real_changed = false;
  
  // Update virtual thermostat state
  if (this->preset != p.id) {
    this->preset = p.id;
    virtual_changed = true;
  }
  
  const float temp = p.getTargetTemperatureForRealClimate();
  if (p.id != manual.id) {
    if (this->target_temperature_low != p.min() || this->target_temperature_high != p.max()) {
      this->target_temperature_low  = p.min();
      this->target_temperature_high = p.max();
      virtual_changed = true;
    }
    if (this->real_climate_->target_temperature != temp) {
      this->real_climate_->target_temperature = temp;
      real_changed = true;
    }
  } else {
    if (this->target_temperature != temp) {
      this->target_temperature = temp;
      virtual_changed = true;
    }
    if (this->real_climate_->target_temperature != temp) {
      this->real_climate_->target_temperature = temp;
      real_changed = true;
    }
  }
  
  auto new_virtual_mode = p.getModeForVirtualThermostat();
  if (this->mode != new_virtual_mode) {
    this->mode = new_virtual_mode;
    virtual_changed = true;
  }
  
  auto new_real_mode = p.getModeForRealClimate();
  if (this->real_climate_->mode != new_real_mode) {
    this->real_climate_->mode = new_real_mode;
    real_changed = true;
  }
  
  auto new_fan_mode = p.getFanModeForRealClimate();
  if (this->real_climate_->fan_mode != new_fan_mode) {
    this->real_climate_->fan_mode = new_fan_mode;
    real_changed = true;
  }
  
  if (real_changed) {
    this->real_climate_->publish_state();
  }
  if (virtual_changed) {
    this->publish_state();
  }
  
  return {virtual_changed, real_changed};
}

const Preset& VirtualThermostat::getActivePreset() const {
  return getActivePresetFromId(this->preset.value_or(climate::CLIMATE_PRESET_NONE));
}

const Preset& VirtualThermostat::getActivePresetFromId(climate::ClimatePreset id) const {
  if (id == home.id)  return home;
  if (id == sleep.id) return sleep;
  if (id == away.id)  return away;
  return manual;  // unknown or empty → manual
}

void VirtualThermostat::control(const climate::ClimateCall &call) {
  bool needs_publish = false;

  if (call.get_fan_mode().has_value()) {
    this->fan_mode = *call.get_fan_mode();
    this->real_climate_->fan_mode = *call.get_fan_mode();
    this->real_climate_->publish_state();
    needs_publish = true;
  }

  // PRESET CHANGE
  if (call.get_preset().has_value()) {
    const auto preset_id = *call.get_preset();
    const auto& active_preset = getActivePresetFromId(preset_id);
    apply_preset(active_preset);
    return; // apply_preset handles publishing
  }

  // MANUAL EDITS → EXIT PRESET MODE
  if (call.get_target_temperature_low().has_value()) {
    this->target_temperature_low = *call.get_target_temperature_low();
    apply_preset(manual);
    return; // apply_preset handles publishing
  }

  if (call.get_target_temperature_high().has_value()) {
    this->target_temperature_high = *call.get_target_temperature_high();
    apply_preset(manual);
    return; // apply_preset handles publishing
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
      needs_publish = true;
    }

    // HEAT/COOL → AUTO (only re-apply preset if in a real preset, not manual)
    if (new_mode == climate::CLIMATE_MODE_AUTO &&
        (this->mode == climate::CLIMATE_MODE_HEAT ||
         this->mode == climate::CLIMATE_MODE_COOL) &&
        active_preset.id != manual.id) {
      apply_preset(active_preset);
      return; // apply_preset handles publishing
    }

    // Update mode and sync to real climate
    this->mode = new_mode;
    this->real_climate_->mode = active_preset.getModeForRealClimate();
    this->real_climate_->target_temperature = active_preset.getTargetTemperatureForRealClimate();
    this->real_climate_->publish_state();
    this->publish_state();
    return;
  }

  if (call.get_target_temperature().has_value()) {
    const float temp = *call.get_target_temperature();
    this->target_temperature = temp;
    this->real_climate_->target_temperature = temp;
    this->real_climate_->publish_state();
    apply_preset(manual);
    return; // apply_preset handles publishing
  }

  if (needs_publish) {
    this->publish_state();
  }
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

