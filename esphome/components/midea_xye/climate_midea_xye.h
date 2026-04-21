#pragma once

#ifdef USE_ARDUINO

#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_traits.h"
#include "esphome/components/number/number.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "ir_transmitter.h"
#include "static_pressure_number.h"
#include "xye.h"
#include "xye_adapter.h"
#include "xye_send.h"
#include "xye_recv.h"

namespace esphome {
namespace midea {
namespace xye {

// Bring XYE protocol types into this namespace
using xye::Command;
using xye::OperationMode;
using xye::FanMode;
using xye::FollowMeSubcommand;
using xye::ControlState;
using xye::TransmitData;
using xye::ReceiveData;

// Legacy compatibility - map old defines to new protocol definitions
// These provide backward compatibility for existing code using old names
constexpr uint8_t CLIENT_COMMAND_QUERY = static_cast<uint8_t>(Command::QUERY);
constexpr uint8_t CLIENT_COMMAND_QUERY_EXTENDED = static_cast<uint8_t>(Command::QUERY_EXTENDED);
constexpr uint8_t CLIENT_COMMAND_SET = static_cast<uint8_t>(Command::SET);
constexpr uint8_t CLIENT_COMMAND_FOLLOWME = static_cast<uint8_t>(Command::FOLLOW_ME);
constexpr uint8_t CLIENT_COMMAND_LOCK = static_cast<uint8_t>(Command::LOCK);
constexpr uint8_t CLIENT_COMMAND_UNLOCK = static_cast<uint8_t>(Command::UNLOCK);
constexpr uint8_t CLIENT_COMMAND_CELSIUS = static_cast<uint8_t>(Command::QUERY_EXTENDED);  // Legacy typo preserved for backward compatibility

constexpr uint8_t OP_MODE_OFF = static_cast<uint8_t>(OperationMode::OFF);
constexpr uint8_t OP_MODE_AUTO = static_cast<uint8_t>(OperationMode::AUTO);
constexpr uint8_t OP_MODE_FAN = static_cast<uint8_t>(OperationMode::FAN);
constexpr uint8_t OP_MODE_DRY = static_cast<uint8_t>(OperationMode::DRY);
constexpr uint8_t OP_MODE_HEAT = static_cast<uint8_t>(OperationMode::HEAT);
constexpr uint8_t OP_MODE_COOL = static_cast<uint8_t>(OperationMode::COOL);

constexpr uint8_t FAN_MODE_AUTO = static_cast<uint8_t>(FanMode::FAN_AUTO);
constexpr uint8_t FAN_MODE_OFF = static_cast<uint8_t>(FanMode::FAN_OFF);
constexpr uint8_t FAN_MODE_HIGH = static_cast<uint8_t>(FanMode::FAN_HIGH);
constexpr uint8_t FAN_MODE_MEDIUM = static_cast<uint8_t>(FanMode::FAN_MEDIUM);
constexpr uint8_t FAN_MODE_LOW = static_cast<uint8_t>(FanMode::FAN_LOW);
// FAN_AUTO_FLAG (0x80) and FAN_SPEED_MASK (0x0F) are defined in xye.h and available in this namespace.

constexpr uint8_t TEMP_SET_FAN_MODE = xye::TEMP_FAN_MODE;

constexpr uint8_t MODE_FLAG_AUX_HEAT = static_cast<uint8_t>(xye::ModeFlags::AUX_HEAT);
constexpr uint8_t MODE_FLAG_NORM = static_cast<uint8_t>(xye::ModeFlags::NORMAL);
constexpr uint8_t MODE_FLAG_ECO = static_cast<uint8_t>(xye::ModeFlags::ECO);
constexpr uint8_t MODE_FLAG_SWING = static_cast<uint8_t>(xye::ModeFlags::SWING);
constexpr uint8_t MODE_FLAG_VENT = static_cast<uint8_t>(xye::ModeFlags::VENTILATION);

constexpr uint8_t TIMER_15MIN = static_cast<uint8_t>(xye::TimerFlags::TIMER_15MIN);
constexpr uint8_t TIMER_30MIN = static_cast<uint8_t>(xye::TimerFlags::TIMER_30MIN);
constexpr uint8_t TIMER_1HOUR = static_cast<uint8_t>(xye::TimerFlags::TIMER_1HOUR);
constexpr uint8_t TIMER_2HOUR = static_cast<uint8_t>(xye::TimerFlags::TIMER_2HOUR);
constexpr uint8_t TIMER_4HOUR = static_cast<uint8_t>(xye::TimerFlags::TIMER_4HOUR);
constexpr uint8_t TIMER_8HOUR = static_cast<uint8_t>(xye::TimerFlags::TIMER_8HOUR);
constexpr uint8_t TIMER_16HOUR = static_cast<uint8_t>(xye::TimerFlags::TIMER_16HOUR);
constexpr uint8_t TIMER_INVALID = static_cast<uint8_t>(xye::TimerFlags::INVALID);

constexpr uint8_t FOLLOWME_SUBCOMMAND_UPDATE = static_cast<uint8_t>(FollowMeSubcommand::UPDATE);
constexpr uint8_t FOLLOWME_SUBCOMMAND_STOP = static_cast<uint8_t>(FollowMeSubcommand::STOP);
constexpr uint8_t FOLLOWME_SUBCOMMAND_INIT = static_cast<uint8_t>(FollowMeSubcommand::INIT);

// SERVER Response compatibility (same as client commands)
constexpr uint8_t SERVER_COMMAND_QUERY = static_cast<uint8_t>(Command::QUERY);
constexpr uint8_t SERVER_COMMAND_SET = static_cast<uint8_t>(Command::SET);
constexpr uint8_t SERVER_COMMAND_LOCK = static_cast<uint8_t>(Command::LOCK);
constexpr uint8_t SERVER_COMMAND_UNLOCK = static_cast<uint8_t>(Command::UNLOCK);

constexpr uint8_t CAPABILITIES_EXT_TEMP = static_cast<uint8_t>(xye::Capabilities::EXTERNAL_TEMP);
constexpr uint8_t CAPABILITIES_SWING = static_cast<uint8_t>(xye::Capabilities::SWING);

constexpr uint8_t OP_FLAG_WATER_PUMP = static_cast<uint8_t>(xye::OperationFlags::WATER_PUMP);
constexpr uint8_t OP_FLAG_WATER_LOCK = static_cast<uint8_t>(xye::OperationFlags::WATER_LOCK);

using climate::ClimateAction;
using climate::ClimateCall;
using climate::ClimateFanMode;
using climate::ClimateMode;
using climate::ClimatePreset;
using climate::ClimateSwingMode;
using sensor::Sensor;

class Constants {
 public:
  static const char *const TAG;
  static const char *const FREEZE_PROTECTION;
  static const char *const SILENT;
  static const char *const TURBO;
};

class ClimateMideaXYE : public PollingComponent, public climate::Climate, public StaticPressureInterface {
 public:
  ClimateMideaXYE() : PollingComponent(1000), tx_data(Command::QUERY) { this->response_timeout = 100; }

#ifdef USE_REMOTE_TRANSMITTER
  void set_transmitter(RemoteTransmitterBase *transmitter) { this->transmitter_.set_transmitter(transmitter); }
#endif

  /* UART communication */

  void set_uart_parent(uart::UARTComponent *parent) { this->uart_ = parent; }
  void set_period(uint32_t ms) { this->set_update_interval(ms); }
  void set_response_timeout(uint32_t ms) { this->response_timeout = ms; }

  /* Component methods */

  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }

  void dump_config() override;
  void set_outdoor_temperature_sensor(Sensor *sensor) { this->outdoor_sensor_ = sensor; }
  void set_temperature_2a_sensor(Sensor *sensor) { this->temperature_2a_sensor_ = sensor; }
  void set_temperature_2b_sensor(Sensor *sensor) { this->temperature_2b_sensor_ = sensor; }
  void set_temperature_3_sensor(Sensor *sensor) { this->temperature_3_sensor_ = sensor; }
  void set_current_sensor(Sensor *sensor) { this->current_sensor_ = sensor; }
  void set_timer_start_sensor(Sensor *sensor) { this->timer_start_sensor_ = sensor; }
  void set_timer_stop_sensor(Sensor *sensor) { this->timer_stop_sensor_ = sensor; }
  void set_error_flags_sensor(Sensor *sensor) { this->error_flags_sensor_ = sensor; }
  void set_protect_flags_sensor(Sensor *sensor) { this->protect_flags_sensor_ = sensor; }
  void set_humidity_setpoint_sensor(Sensor *sensor) { this->humidity_sensor_ = sensor; }
  void set_power_sensor(Sensor *sensor) { this->power_sensor_ = sensor; }
  void set_follow_me_sensor(Sensor *sensor);
  void set_internal_current_temperature_sensor(Sensor *sensor) { this->internal_current_temperature_sensor_ = sensor; }
  void set_use_fahrenheit(bool yesno) { this->use_fahrenheit_ = yesno; }
  void set_static_pressure_number(StaticPressureNumber *number) {
    this->static_pressure_number_ = number;
    number->set_parent(this);
  }
  void set_static_pressure(uint8_t value) override;
  void update() override;
  void setup() override;
  void loop() override {}
  void sendRecv(uint8_t cmdSent);
  void setPowerState(bool state);
  void setTransmitParams();

  /* ############### */
  /* ### ACTIONS ### */
  /* ############### */

  void do_follow_me(float temperature, bool beeper = false);
  void do_display_toggle();
  void do_swing_step();
  // TODO: Do we actually need these three?
  void do_power_on() { this->setPowerState(true); }
  void do_power_off() { this->setPowerState(false); }
  void do_power_toggle() { this->setPowerState(this->mode == ClimateMode::CLIMATE_MODE_OFF); }

  void set_supported_modes(climate::ClimateModeMask modes) { this->supported_modes_ = modes; }
  void set_supported_swing_modes(climate::ClimateSwingModeMask modes) { this->supported_swing_modes_ = modes; }
  void set_supported_presets(climate::ClimatePresetMask presets) { this->supported_presets_ = presets; }
  void set_custom_presets(std::vector<const char *> presets) { this->supported_custom_presets_ = presets; }
  void set_custom_fan_modes(std::vector<const char *> modes) { this->supported_custom_fan_modes_ = modes; }

  // Protocol message buffers - use unions for structured access
  TransmitData tx_data;
  ReceiveData rx_data;

 private:
  ControlState controlState;
  uint8_t ForceReadNextCycle;
  ControlState queuedCommand;
  uint32_t response_timeout;
  // Tracks whether Follow-Me has been initialized after mode change.
  // When false, the next Follow-Me update sends an INIT subcommand (0x06).
  // When true, Follow-Me updates send a regular UPDATE subcommand (0x02).
  bool followMeInit;
  uint8_t lastFollowMeTemperature;
  // Number of upcoming QUERY responses whose reported mode should be
  // ignored after we issue a SET. Prevents a transient mode flap
  // (e.g. cool -> off -> cool) while the unit acknowledges the new mode.
  uint8_t post_set_grace_{0};

 protected:
  uart::UARTComponent *uart_;
#ifdef USE_REMOTE_TRANSMITTER
  IrTransmitter transmitter_;
#endif
  void control(const ClimateCall &call) override;
  climate::ClimateTraits traits() override;
  climate::ClimateModeMask supported_modes_{};
  climate::ClimateSwingModeMask supported_swing_modes_{};
  climate::ClimatePresetMask supported_presets_{};
  std::vector<const char *> supported_custom_presets_{};
  std::vector<const char *> supported_custom_fan_modes_{};
  bool use_fahrenheit_;
  Sensor *outdoor_sensor_{nullptr};
  Sensor *temperature_2a_sensor_{nullptr};
  Sensor *temperature_2b_sensor_{nullptr};
  Sensor *temperature_3_sensor_{nullptr};
  Sensor *current_sensor_{nullptr};
  Sensor *timer_start_sensor_{nullptr};
  Sensor *timer_stop_sensor_{nullptr};
  Sensor *error_flags_sensor_{nullptr};
  Sensor *protect_flags_sensor_{nullptr};
  Sensor *humidity_sensor_{nullptr};
  Sensor *power_sensor_{nullptr};
  Sensor *follow_me_sensor_{nullptr};
  Sensor *internal_current_temperature_sensor_{nullptr};
  StaticPressureNumber *static_pressure_number_{nullptr};
  ClimateMode last_on_mode_;
  float internal_temperature_{NAN};

  void ParseResponse();
  uint8_t CalculateSetTime(uint32_t time);
  uint32_t CalculateGetTime(uint8_t time);
  void update_current_temperature_from_sensors_(bool &need_publish);
  void on_follow_me_sensor_update_(float state);
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
