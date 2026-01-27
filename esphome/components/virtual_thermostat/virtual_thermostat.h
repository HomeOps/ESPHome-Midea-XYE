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
  bool updating_{false};  // Guard flag to prevent recursive updates
  void min_entity(number::Number *n);
  void max_entity(number::Number *n);

  float min() const;
  float max() const;

  float getTargetTemperatureForRealClimate() const;

  float getCurrentRoomTemperatureForRealClimate() const;

  climate::ClimateFanMode getFanModeForRealClimate() const;

  climate::ClimateMode getModeForVirtualThermostat() const;

  climate::ClimateMode getModeForRealClimate() const;

  void on_min_changed(float new_min);
  void on_max_changed(float new_max);
};

class VirtualThermostat : public climate::Climate, public Component {
friend class Preset;
public:
  // External inputs (wired from YAML/codegen)
  sensor::Sensor *room_sensor_{nullptr};
  void room_sensor(sensor::Sensor *s) { this->room_sensor_ = s; }
  climate::Climate *real_climate_{nullptr};
  void real_climate(climate::Climate *c) { this->real_climate_ = c; }
  
  // Update interval (configurable from YAML)
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
  void apply_preset(const Preset& p);
  const Preset& getActivePreset() const;
  const Preset& getActivePresetFromId(climate::ClimatePreset id) const;
  void exit_preset_mode();
  void update_real_climate();
  
  uint32_t update_interval_ms_{30000}; // Default 30 seconds
  uint32_t last_update_time_{0};
};

}  // namespace virtual_thermostat
}  // namespace esphome

