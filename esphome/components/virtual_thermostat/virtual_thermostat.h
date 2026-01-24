#pragma once

#include "esphome.h"

namespace esphome {
namespace virtual_thermostat {

// -------------------------
// PRESET STRUCT
// -------------------------
struct Preset {
  std::string name;
  number::Number *min_entity;
  number::Number *max_entity;

  float min() const {
    return min_entity ? min_entity->state : NAN;
  }

  float max() const {
    return max_entity ? max_entity->state : NAN;
  }

  float getTemp() const {
    return (min() + max()) / 2.0f;
  }
};

// -------------------------
// MAIN CLASS
// -------------------------
class VirtualThermostat : public climate::Climate, public Component {
 public:
  // External inputs
  sensor::Sensor *room_sensor = nullptr;
  climate::Climate *real_ac = nullptr;

  // Presets (entities are assigned via YAML/codegen)
  Preset home;
  Preset sleep;
  Preset away;

  // Manual preset (no entities, stores its own values)
  Preset manual;
  float manual_min = 0.0f;
  float manual_max = 0.0f;

  // Active preset pointer
  Preset *active_preset = nullptr;

  // Last preset name (for remembering selection)
  std::string last_preset_name;

  // Constructor
  VirtualThermostat();

  // ESPHome overrides
  void setup() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void loop() override;

 private:
  void apply_preset(Preset *p);
  void exit_preset_mode();
  Preset *getPresetFromName(const std::string &name);
};

}  // namespace virtual_thermostat
}  // namespace esphome

