#pragma once

#include "esphome.h"

namespace esphome {
namespace virtual_thermostat {

struct Preset {
  std::string name;
  number::Number *min_entity{nullptr};
  number::Number *max_entity{nullptr};

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

class VirtualThermostat : public climate::Climate, public Component {
 public:
  // External inputs (wired from YAML/codegen)
  sensor::Sensor *room_sensor{nullptr};
  climate::Climate *real_ac{nullptr};

  // Presets (entities wired from YAML/codegen)
  Preset home;
  Preset sleep;
  Preset away;

  // Manual preset (no entities, stores its own values)
  Preset manual;
  float manual_min{0.0f};
  float manual_max{0.0f};

  // Active preset pointer
  Preset *active_preset{nullptr};

  // Last preset name (RAM; persistence can be added later)
  std::string last_preset_name;

  VirtualThermostat();

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

