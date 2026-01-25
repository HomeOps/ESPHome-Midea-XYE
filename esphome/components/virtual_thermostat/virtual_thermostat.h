#pragma once

#include "esphome.h"

namespace esphome {
namespace virtual_thermostat {

class VirtualThermostat;

struct Preset {
  Preset() = delete;
  Preset(climate::ClimatePreset id, VirtualThermostat *thermostat) : id(id), thermostat(thermostat) {}
  climate::ClimatePreset id;
  number::Number *min_entity_{nullptr};
  number::Number *max_entity_{nullptr};
  VirtualThermostat *thermostat{nullptr};
  void min_entity(number::Number *n);
  void max_entity(number::Number *n);

  float min() const;
  float max() const;
  float getTemp() const;
};

class VirtualThermostat : public climate::Climate, public Component {
friend class Preset;
public:
  // External inputs (wired from YAML/codegen)
  sensor::Sensor *room_sensor_{nullptr};
  void room_sensor(sensor::Sensor *s) { this->room_sensor_ = s; }
  climate::Climate *real_climate_{nullptr};
  void real_climate(climate::Climate *c) { this->real_climate_ = c; }

  // Presets (entities wired from YAML/codegen)
  Preset home { climate::CLIMATE_PRESET_HOME, this };
  Preset sleep { climate::CLIMATE_PRESET_SLEEP, this };
  Preset away { climate::CLIMATE_PRESET_AWAY, this };
  Preset manual { climate::CLIMATE_PRESET_NONE, this };

  VirtualThermostat();

  
  void setup() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void loop() override;

 private:
  void apply_preset(const Preset& p);
  void exit_preset_mode();
  const Preset& getActivePreset() const;
};

}  // namespace virtual_thermostat
}  // namespace esphome

