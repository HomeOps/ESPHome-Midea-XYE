#include "virtual_thermostat.h"

namespace esphome {
namespace virtual_thermostat {

VirtualThermostat::VirtualThermostat() {
  // Only set names; do NOT touch min_entity/max_entity (they come from YAML/codegen)
  home.name  = "Home";
  sleep.name = "Sleep";
  away.name  = "Away";

  manual.name = "";  // manual mode
  active_preset = &manual;
  last_preset_name.clear();
}

void VirtualThermostat::setup() {
  this->mode = climate::CLIMATE_MODE_AUTO;

  // Restore last preset in RAM (persistence can be added later)
  active_preset = getPresetFromName(last_preset_name);

  // If active preset is not manual and has entities, apply it
  if (active_preset != &manual &&
      active_preset->min_entity &&
      active_preset->max_entity) {
    apply_preset(active_preset);
  } else {
    // Fallback: use manual range
    this->target_temperature_low  = manual_min;
    this->target_temperature_high = manual_max;
  }
}

climate::ClimateTraits VirtualThermostat::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_auto_mode(true);
  traits.set_supports_heat_mode(true);
  traits.set_supports_cool_mode(true);
  traits.set_supports_two_point_target_temperature(true);
  traits.set_supports_fan_modes(true);
  traits.set_supports_presets(true);
  return traits;
}

void VirtualThermostat::apply_preset(Preset *p) {
  if (p == &manual) {
    this->target_temperature_low  = manual_min;
    this->target_temperature_high = manual_max;
  } else if (p->min_entity && p->max_entity) {
    this->target_temperature_low  = p->min();
    this->target_temperature_high = p->max();
  }
}

void VirtualThermostat::exit_preset_mode() {
  active_preset = &manual;
  manual_min = this->target_temperature_low;
  manual_max = this->target_temperature_high;
  last_preset_name.clear();
}

Preset *VirtualThermostat::getPresetFromName(const std::string &name) {
  if (name == home.name)  return &home;
  if (name == sleep.name) return &sleep;
  if (name == away.name)  return &away;
  return &manual;  // unknown or empty → manual
}

void VirtualThermostat::control(const climate::ClimateCall &call) {
  // MODE CHANGE
  if (call.get_mode().has_value()) {
    auto new_mode = *call.get_mode();

    // AUTO → HEAT/COOL
    if ((new_mode == climate::CLIMATE_MODE_HEAT ||
         new_mode == climate::CLIMATE_MODE_COOL) &&
        this->mode == climate::CLIMATE_MODE_AUTO) {

      if (active_preset == &manual) {
        this->target_temperature = (manual_min + manual_max) / 2.0f;
      } else {
        this->target_temperature = active_preset->getTemp();
      }
    }

    // HEAT/COOL → AUTO
    if (new_mode == climate::CLIMATE_MODE_AUTO &&
        (this->mode == climate::CLIMATE_MODE_HEAT ||
         this->mode == climate::CLIMATE_MODE_COOL)) {

      if (active_preset != &manual) {
        apply_preset(active_preset);
      }
      // manual → keep current min/max
    }

    this->mode = new_mode;
  }

  // PRESET CHANGE
  if (call.get_preset().has_value()) {
    std::string p = *call.get_preset();

    active_preset = getPresetFromName(p);
    last_preset_name = (active_preset == &manual) ? "" : active_preset->name;

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

  float room = this->room_sensor->state;

  // AUTO MODE
  if (this->mode == climate::CLIMATE_MODE_AUTO) {
    int minv = (int)this->target_temperature_low;
    int maxv = (int)this->target_temperature_high;

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
    int t = (int)this->target_temperature;
    this->action = climate::CLIMATE_ACTION_HEATING;
    this->real_ac->mode = climate::CLIMATE_MODE_HEAT;
    this->real_ac->target_temperature = t;
  }

  // COOL MODE
  if (this->mode == climate::CLIMATE_MODE_COOL) {
    int t = (int)this->target_temperature;
    this->action = climate::CLIMATE_ACTION_COOLING;
    this->real_ac->mode = climate::CLIMATE_MODE_COOL;
    this->real_ac->target_temperature = t;
  }

  // FAN MODE PASSTHROUGH
  if (this->fan_mode == "Auto")
    this->real_ac->fan_mode = climate::CLIMATE_FAN_AUTO;
  else if (this->fan_mode == "Low")
    this->real_ac->fan_mode = climate::CLIMATE_FAN_LOW;
  else if (this->fan_mode == "Med")
    this->real_ac->fan_mode = climate::CLIMATE_FAN_MEDIUM;
  else if (this->fan_mode == "High")
    this->real_ac->fan_mode = climate::CLIMATE_FAN_HIGH;

  this->publish_state();
}

}  // namespace virtual_thermostat
}  // namespace esphome

