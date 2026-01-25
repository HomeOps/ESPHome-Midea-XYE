#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

float Preset::min() const {
  return min_entity ? min_entity->state : thermostat->target_temperature_low;
}

float Preset::max() const {
  return max_entity ? max_entity->state : thermostat->target_temperature_high;
}

float Preset::getTemp() const {
  return (min() + max()) / 2.0f;
}

VirtualThermostat::VirtualThermostat() {
}

void VirtualThermostat::setup() {
  const auto& active_preset = getActivePreset();
  apply_preset(active_preset);
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
  this->target_temperature_low  = p.min();
  this->target_temperature_high = p.max();
}

void VirtualThermostat::exit_preset_mode() {
  this->preset = manual.id;
}

const Preset& VirtualThermostat::getActivePreset() const {
  const auto id = this->preset;
  if (id == home.id)  return home;
  if (id == sleep.id) return sleep;
  if (id == away.id)  return away;
  return manual;  // unknown or empty → manual
}

void VirtualThermostat::control(const climate::ClimateCall &call) {
  // MODE CHANGE
  if (call.get_mode().has_value()) {
    const auto new_mode = *call.get_mode();

    const auto& active_preset = getActivePreset();

    // AUTO → HEAT/COOL
    if ((new_mode == climate::CLIMATE_MODE_HEAT ||
         new_mode == climate::CLIMATE_MODE_COOL) &&
        this->mode == climate::CLIMATE_MODE_AUTO) {
      this->target_temperature = active_preset.getTemp();
    }

    // HEAT/COOL → AUTO
    if (new_mode == climate::CLIMATE_MODE_AUTO &&
        (this->mode == climate::CLIMATE_MODE_HEAT ||
         this->mode == climate::CLIMATE_MODE_COOL)) {
      apply_preset(active_preset);
    }

    this->mode = new_mode;
  }

  // PRESET CHANGE
  if (call.get_preset().has_value()) {
    const auto& active_preset = getActivePreset();
    apply_preset(active_preset);
  }

  // MANUAL EDITS → EXIT PRESET MODE
  if (call.get_target_temperature_low().has_value()) {
    this->target_temperature_low = *call.get_target_temperature_low();
    exit_preset_mode();
  }

  if (call.get_target_temperature_high().has_value()) {
    this->target_temperature_high = *call.get_target_temperature_high();
    exit_preset_mode();
  }

  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    exit_preset_mode();
  }

  // FAN MODE
  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();

  this->publish_state();
}

void VirtualThermostat::loop() {
  if (!this->room_sensor || !this->real_ac) return;

  const float room = this->room_sensor->state;

  // AUTO MODE
  if (this->mode == climate::CLIMATE_MODE_AUTO) {
    const auto minv = this->target_temperature_low;
    const auto maxv = this->target_temperature_high;

    if (room < minv) {
      this->action = climate::CLIMATE_ACTION_HEATING;
      this->real_ac->mode = climate::CLIMATE_MODE_HEAT;
      this->real_ac->target_temperature = minv;
    }
    else if (room > maxv) {
      this->action = climate::CLIMATE_ACTION_COOLING;
      this->real_ac->mode = climate::CLIMATE_MODE_COOL;
      this->real_ac->target_temperature = maxv;
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
    this->real_ac->mode = climate::CLIMATE_MODE_HEAT;
    this->real_ac->target_temperature = t;
  }

  // COOL MODE
  if (this->mode == climate::CLIMATE_MODE_COOL) {
    const auto t = this->target_temperature;
    this->action = climate::CLIMATE_ACTION_COOLING;
    this->real_ac->mode = climate::CLIMATE_MODE_COOL;
    this->real_ac->target_temperature = t;
  }

  // FAN MODE PASSTHROUGH
  if (this->fan_mode == climate::CLIMATE_FAN_AUTO)
    this->real_ac->fan_mode = climate::CLIMATE_FAN_AUTO;
  else if (this->fan_mode == climate::CLIMATE_FAN_LOW)
    this->real_ac->fan_mode = climate::CLIMATE_FAN_LOW;
  else if (this->fan_mode == climate::CLIMATE_FAN_MEDIUM)
    this->real_ac->fan_mode = climate::CLIMATE_FAN_MEDIUM;
  else if (this->fan_mode == climate::CLIMATE_FAN_HIGH)
    this->real_ac->fan_mode = climate::CLIMATE_FAN_HIGH;

  this->publish_state();
}

}  // namespace virtual_thermostat
}  // namespace esphome

