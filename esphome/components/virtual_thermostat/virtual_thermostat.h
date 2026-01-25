#pragma once

#include "esphome.h"

namespace esphome {
namespace virtual_thermostat {

struct Preset {
  climate::ClimatePreset id{climate::CLIMATE_PRESET_NONE};
  number::Number *min_entity{nullptr};
  float min_ = NAN;
  number::Number *max_entity{nullptr};
  float max_ = NAN;

  float min() const {
    return min_entity ? min_entity->state : min_;
  }

  float max() const {
    return max_entity ? max_entity->state : max_;
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
  Preset home { climate::CLIMATE_PRESET_HOME };
  Preset sleep { climate::CLIMATE_PRESET_SLEEP };
  Preset away { climate::CLIMATE_PRESET_AWAY };

  // Manual preset (no entities, stores its own values)
  Preset manual { climate::CLIMATE_PRESET_NONE, nullptr, 20.0f, nullptr, 23.0f };

  // Last preset name (RAM; persistence can be added later)
  climate::ClimatePreset last_preset_id {manual.id};

  VirtualThermostat();

  void setup() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void loop() override;

 private:
  void apply_preset(const Preset& p);
  void exit_preset_mode();
  const Preset& getPresetFromId(const climate::ClimatePreset &id) const;
};

}  // namespace virtual_thermostat
}  // namespace esphome

