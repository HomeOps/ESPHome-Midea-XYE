#pragma once

#include "esphome.h"
#include "preset.h"

namespace esphome {
namespace virtual_thermostat {

class VirtualThermostat : public climate::Climate, public Component {
friend class Preset;
public:
  // External inputs (wired from YAML/codegen)
  sensor::Sensor *room_sensor_{nullptr};
  void room_sensor(sensor::Sensor *s) { this->room_sensor_ = s; }
  climate::Climate *real_climate_{nullptr};
  void real_climate(climate::Climate *c) { this->real_climate_ = c; }
  
  // Update interval (configured from YAML via codegen) for periodic sync with real climate
  void set_update_interval(uint32_t interval_ms) { this->update_interval_ms_ = interval_ms; }

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
  std::pair<bool, bool> apply_preset(const Preset& p);
  const Preset& getActivePreset() const;
  const Preset& getActivePresetFromId(climate::ClimatePreset id) const;
  void update_real_climate();
  
  uint32_t update_interval_ms_{30000}; // Default 30 seconds
  uint32_t last_update_time_{0};
};

}  // namespace virtual_thermostat
}  // namespace esphome

