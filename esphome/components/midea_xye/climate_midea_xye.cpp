#ifdef USE_ARDUINO

#include "climate_midea_xye.h"
#include "build_info.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

const char *const Constants::TAG = "midea_xye";
const char *const Constants::FREEZE_PROTECTION = "Freeze Protection";
const char *const Constants::SILENT = "Silent";
const char *const Constants::TURBO = "Turbo";

static void set_sensor(Sensor *sensor, float value) {
  if (sensor == nullptr)
    return;
  if (std::isnan(value)) {
    if (!sensor->has_state() || !std::isnan(sensor->get_raw_state()))
      sensor->publish_state(value);
    return;
  }
  if (!sensor->has_state() || sensor->get_raw_state() != value)
    sensor->publish_state(value);
}

static void set_number(number::Number *number, float value) {
  if (number != nullptr && (!number->has_state() || number->state != value))
    number->publish_state(value);
}

#ifdef USE_BINARY_SENSOR
static void set_binary_sensor(binary_sensor::BinarySensor *sens, bool value) {
  if (sens != nullptr && (!sens->has_state() || sens->state != value))
    sens->publish_state(value);
}
#endif

template<typename T> void update_property(T &property, const T &value, bool &flag) {
  if (property != value) {
    property = value;
    flag = true;
  }
}

float ClimateMideaXYE::decode_bus_temperature_(uint8_t raw) const {
  if (raw == 0xFF)
    return NAN;
  if (this->raw_fahrenheit_temperatures_) {
    return (static_cast<float>(raw) - 32.0f) * 5.0f / 9.0f;
  }
  return XYEAdapter::get_temperature(raw);
}

float ClimateMideaXYE::decode_target_temperature_(uint8_t raw) const {
  if (this->raw_fahrenheit_temperatures_) {
    // Same sentinel handling for target fields observed on unsupported frames.
    if (raw == 0xFF)
      return NAN;
    return (static_cast<float>(raw) - 32.0f) * 5.0f / 9.0f;
  }
  return XYEAdapter::get_target_temperature(raw, this->use_fahrenheit_);
}

uint8_t ClimateMideaXYE::encode_target_temperature_(float celsius) const {
  if (this->raw_fahrenheit_temperatures_) {
    if (std::isnan(celsius))
      return 0;
    const int fahrenheit = static_cast<int>(lroundf((9.0f / 5.0f) * celsius + 32.0f));
    if (fahrenheit < 0)
      return 0;
    if (fahrenheit > 255)
      return 255;
    return static_cast<uint8_t>(fahrenheit);
  }
  return XYEAdapter::get_raw_target_temperature(celsius, this->use_fahrenheit_);
}

void ClimateMideaXYE::control(const ClimateCall &call) {
  if (call.get_mode().has_value()) {
    this->mode = call.get_mode().value();
    // Reset Follow-Me initialization flag when mode changes to ensure
    // proper initialization sequence is sent on next Follow-Me update
    followMeInit = false;
  }
  if (call.get_target_temperature().has_value())
    this->target_temperature = call.get_target_temperature().value();
  if (call.get_fan_mode().has_value())
    this->fan_mode = call.get_fan_mode().value();
  if (call.get_swing_mode().has_value())
    this->swing_mode = call.get_swing_mode().value();
  if (call.get_preset().has_value())
    this->preset = call.get_preset().value();
  this->publish_state();

  if (controlState != ControlState::WAIT_DATA) {
    controlState = ControlState::SEND_SET;
  } else {
    queuedCommand = ControlState::SEND_SET;
  }
}

void ClimateMideaXYE::setup() {
  // this->uart_->check_uart_settings(4800, 1, UART_CONFIG_PARITY_NONE, 8);
  this->last_on_mode_ = *this->supported_modes_.begin();
  controlState = ControlState::SEND_QUERY;
  queuedCommand = ControlState::WAIT_DATA;
  ForceReadNextCycle = 1;
  followMeInit = false;

  // Register custom modes on the Climate base class. ESPHome 2026.4.0
  // deprecated the equivalent ClimateTraits setters in favor of these.
  this->set_supported_custom_presets(this->supported_custom_presets_);
  this->set_supported_custom_fan_modes(this->supported_custom_fan_modes_);

  // Start up in Auto fan mode (since unit doesn't report it correctly)
  this->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
}

void ClimateMideaXYE::set_follow_me_sensor(Sensor *sensor) {
  this->follow_me_sensor_ = sensor;
  if (sensor != nullptr) {
    sensor->add_on_state_callback([this](float state) { this->on_follow_me_sensor_update_(state); });
  }
}

// TODO: Not sure if we really need this.
void ClimateMideaXYE::setPowerState(bool state) {
  if (state)
    this->mode = this->last_on_mode_;
  else
    this->mode = ClimateMode::CLIMATE_MODE_OFF;

  if (controlState != ControlState::WAIT_DATA) {
    controlState = ControlState::SEND_SET;
  } else {
    queuedCommand = ControlState::SEND_SET;
  }
}

void ClimateMideaXYE::setTransmitParams() {
  tx_data = TransmitData(Command::SET);
  auto &d = tx_data.message.data.standard;

  d.operation_mode = XYEAdapter::get_operation_mode(this->mode);

  if (this->mode != ClimateMode::CLIMATE_MODE_HEAT_COOL) {
    d.fan_mode = XYEAdapter::get_fan_mode(this->fan_mode.value());
  } else {
    // Auto is full-auto - can't set fan mode either.
    this->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
    d.fan_mode = FanMode::FAN_AUTO;
  }

  d.target_temperature.value = this->encode_target_temperature_(this->target_temperature);

  d.mode_flags = XYEAdapter::get_mode_flags(
      this->preset.value_or(ClimatePreset::CLIMATE_PRESET_NONE), this->swing_mode);

  tx_data.update_crc();
}

void ClimateMideaXYE::sendRecv(uint8_t cmdSent) {
  // TODO: Reimplement flow control for manual RS485 flow control chips
  // digitalWrite(ComControlPin, RS485_TX_PIN_VALUE);
  // Log outgoing message at debug level
  tx_data.print_debug(Constants::TAG, TX_MESSAGE_LENGTH, ESPHOME_LOG_LEVEL_DEBUG);
  this->uart_->write_array(tx_data.raw, TX_MESSAGE_LENGTH);
  this->uart_->flush();
  controlState = ControlState::WAIT_DATA;
  // Delay for response_timeout ms to allow response from the AC unit.
  this->set_timeout("read-result", this->response_timeout, [this, cmdSent]() {
    // digitalWrite(ComControlPin, RS485_RX_PIN_VALUE);

    uint8_t i = 0;
    while (this->uart_->available()) {
      if (i < RX_MESSAGE_LENGTH)
        this->uart_->read_byte(&rx_data.raw[i]);
      i++;
    }
    if (i == RX_MESSAGE_LENGTH) {
      // Log incoming message at debug level
      rx_data.print_debug(i, Constants::TAG, ESPHOME_LOG_LEVEL_DEBUG, this->use_fahrenheit_,
                          this->raw_fahrenheit_temperatures_);
      // Don't parse responses to SET or FOLLOW_ME commands to avoid
      // overwriting the mode we just set. The AC state will be updated
      // on subsequent QUERY cycles.
      if (cmdSent != CLIENT_COMMAND_SET && cmdSent != CLIENT_COMMAND_FOLLOWME) {
        ParseResponse();
      }
      if (queuedCommand != ControlState::WAIT_DATA) {
        controlState = queuedCommand;
        queuedCommand = ControlState::WAIT_DATA;
      } else {
        switch (cmdSent) {
          case CLIENT_COMMAND_QUERY:
            controlState = ControlState::SEND_QUERY_EXTENDED;
            break;
          case CLIENT_COMMAND_SET:
            controlState = ControlState::SEND_FOLLOWME;
            break;
          case CLIENT_COMMAND_QUERY_EXTENDED:
            controlState = ControlState::SEND_QUERY;
            break;
          case CLIENT_COMMAND_FOLLOWME:
            controlState = ControlState::SEND_QUERY;
            break;
        }
      }
    } else {
      ESP_LOGE(Constants::TAG, "Received incorrect message length from AC for Command %02X", cmdSent);
      rx_data.print_debug(i, Constants::TAG, ESPHOME_LOG_LEVEL_ERROR);
    }
  });
}

void ClimateMideaXYE::update() {
  uint8_t cmdSent = 0x00;
  // Possible States:
  // 0: Waiting for Response from Command
  // 1: Sending Set C3 Command
  // 2: Sending Set C6 Command
  // 3: Sending Query C0 Command
  // 4: Sending Query C4 Command
  switch (controlState) {
    case ControlState::SEND_SET: {
      setTransmitParams();
      post_set_grace_ = 2;
      cmdSent = CLIENT_COMMAND_SET;
      sendRecv(cmdSent);
      break;
    }
    case ControlState::SEND_FOLLOWME: {
      // If the AC mode changed, follow-me should be
      // refreshed, if emulating the wired controller's
      // behavior.
      cmdSent = CLIENT_COMMAND_FOLLOWME;
      sendRecv(cmdSent);
      if (this->mode == ClimateMode::CLIMATE_MODE_OFF) {
        ESP_LOGI(Constants::TAG, "Set static pressure.");
      } else {
        ESP_LOGI(Constants::TAG, "Sent Follow-Me data.");
      }
      break;
    }
    case ControlState::SEND_QUERY: {
      tx_data = TransmitData(Command::QUERY);
      tx_data.update_crc();
      cmdSent = CLIENT_COMMAND_QUERY;
      sendRecv(cmdSent);
      break;
    }
    case ControlState::SEND_QUERY_EXTENDED: {
      tx_data = TransmitData(Command::QUERY_EXTENDED);
      tx_data.update_crc();
      cmdSent = CLIENT_COMMAND_QUERY_EXTENDED;
      sendRecv(cmdSent);
      break;
    }
    case ControlState::WAIT_DATA: {
      // Wait for data to processed. Do nothing during the loop.
      break;
    }
  }
}

void ClimateMideaXYE::ParseResponse() {
  if (!rx_data.is_valid()) {
    ESP_LOGE(Constants::TAG, "Received invalid response from AC");
    rx_data.print_debug(RX_MESSAGE_LENGTH, Constants::TAG, ESPHOME_LOG_LEVEL_ERROR);
    return;
  }

  switch (rx_data.message.frame.header.command) {
    case Command::QUERY: {
      const auto &qr = rx_data.message.data.query_response;
      ClimatePreset preset = ClimatePreset::CLIMATE_PRESET_NONE;

      const ClimateMode mode = XYEAdapter::get_climate_mode(qr.operation_mode);
      const ClimateFanMode fan_mode = XYEAdapter::get_climate_fan_mode(qr.fan_mode);

      if (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_AUX_HEAT)
        preset = ClimatePreset::CLIMATE_PRESET_BOOST;
      else if (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_ECO)
        preset = ClimatePreset::CLIMATE_PRESET_SLEEP;

      bool need_publish = false;

      if (post_set_grace_ > 0) {
        post_set_grace_--;
        ESP_LOGD(Constants::TAG,
                 "Post-SET grace: ignoring reported mode=%d (%u cycle(s) remaining)",
                 static_cast<int>(mode), post_set_grace_);
      } else {
        update_property(this->mode, mode, need_publish);
        if (mode != ClimateMode::CLIMATE_MODE_OFF) {
          this->last_on_mode_ = mode;
        }
      }

      if (mode != ClimateMode::CLIMATE_MODE_OFF || ForceReadNextCycle == 1) {
        // Store the internal temperature from the XYE bus
        this->internal_temperature_ = this->decode_bus_temperature_(qr.t1_temperature.value);

        // Publish the internal temperature to the sensor if configured
        set_sensor(this->internal_current_temperature_sensor_, this->internal_temperature_);

        // Update current_temperature based on sensor availability
        this->update_current_temperature_from_sensors_(need_publish);

        if (this->target_temperature_from_c0_ && post_set_grace_ == 0) {
          const float incoming_target_temp = this->decode_target_temperature_(qr.target_temperature.value);
          if (!std::isnan(incoming_target_temp))
            update_property(this->target_temperature, incoming_target_temp, need_publish);
        }

        // Compressor/defrost-aware action is opt-in (compressor_aware_action) while the
        // C0 byte-19 compressor flag is still provisional. When disabled, compressor_active=true
        // and defrost_active=false reproduce the legacy "fan running implies heating/cooling".
        const bool compressor_active = !this->compressor_aware_action_ ||
                                       qr.compressor_running_flag == CompressorRunningFlag::ACTIVE;
        const bool defrost_active = this->compressor_aware_action_ &&
                                    XYEAdapter::is_defrost_active(qr.protect_flags.value());
        update_property(this->action,
                        XYEAdapter::get_climate_action(mode, qr.fan_mode, qr.operation_mode,
                                                       compressor_active, defrost_active),
                        need_publish);

        if ((this->swing_mode != ClimateSwingMode::CLIMATE_SWING_OFF) !=
            (bool) (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_SWING))
          need_publish = true;
        this->swing_mode = (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_SWING)
                               ? ClimateSwingMode::CLIMATE_SWING_VERTICAL
                               : ClimateSwingMode::CLIMATE_SWING_OFF;
        if (this->preset != preset)
          need_publish = true;
        this->preset = preset;
      } else if ((this->action != climate::CLIMATE_ACTION_IDLE) &&
                 (static_cast<uint8_t>(qr.fan_mode) & FAN_SPEED_MASK) == 0x00) {
        this->action = climate::CLIMATE_ACTION_IDLE;
        need_publish = true;
      }

      if (need_publish)
        this->publish_state();

      set_sensor(this->temperature_2a_sensor_, this->decode_bus_temperature_(qr.t2a_temperature.value));
      set_sensor(this->temperature_2b_sensor_, this->decode_bus_temperature_(qr.t2b_temperature.value));
      set_sensor(this->temperature_3_sensor_, this->decode_bus_temperature_(qr.t3_temperature.value));
      set_sensor(this->current_sensor_, static_cast<float>(qr.current));
      set_sensor(this->timer_start_sensor_, CalculateGetTime(qr.timer_start));
      set_sensor(this->timer_stop_sensor_, CalculateGetTime(qr.timer_stop));
      set_sensor(this->error_flags_sensor_, static_cast<float>(qr.error_flags.value()));
      set_sensor(this->protect_flags_sensor_, static_cast<float>(qr.protect_flags.value()));
      set_sensor(this->fan_speed_sensor_, static_cast<float>(XYEAdapter::get_fan_speed_level(qr.fan_mode)));
#ifdef USE_BINARY_SENSOR
      set_binary_sensor(this->defrost_sensor_, XYEAdapter::is_defrost_active(qr.protect_flags.value()));
      set_binary_sensor(this->compressor_active_sensor_,
                        qr.compressor_running_flag == CompressorRunningFlag::ACTIVE);
#endif
      break;
    }
    case Command::QUERY_EXTENDED: {
      bool need_publish = false;
      const auto &exr = rx_data.message.data.extended_query_response;
      set_sensor(this->outdoor_sensor_, this->decode_bus_temperature_(exr.outdoor_temperature.value));
      set_number(this->static_pressure_number_, static_cast<float>(STATIC_PRESSURE_VALUE_MASK & exr.static_pressure));
      // C4 is the default source for target_temperature, covering both standard unit modes:
      //  - Fahrenheit: encoded as (°F + FAHRENHEIT_TEMP_OFFSET); convert to Celsius for ESPHome.
      //  - Celsius:    raw integer degrees with bit 6 (0x40) status flag; mask before use.
      // Units that expose setpoint only on C0 can opt into target_temperature_source: C0.
      // Respect post_set_grace_: skip until the C0 grace window has closed so a freshly-sent
      // SET command isn't immediately overwritten by stale device state.
      // Fahrenheit C4 decode approach adapted from rmounce/esphome@xye-units-switch.
      if (!this->target_temperature_from_c0_ &&
          (this->mode != ClimateMode::CLIMATE_MODE_OFF || ForceReadNextCycle == 1) &&
          post_set_grace_ == 0) {
        const float incoming_target_temp = this->decode_target_temperature_(exr.target_temperature.value);
        if (!std::isnan(incoming_target_temp))
          update_property(this->target_temperature, incoming_target_temp, need_publish);
      }
      if (need_publish)
        this->publish_state();
      // Sync fan mode from the C4 target_fan_speed field when enabled. This is the commanded
      // speed as set on the physical thermostat and persists when the fan is idle, unlike
      // C0 fan_mode which reads 0x00 when stopped.
      // Respect post_set_grace_: if a SET was just issued the device may not yet have
      // updated its reported target_fan_speed, so skip until C0 has cleared the grace window.
      if (this->sync_fan_mode_from_device_ && post_set_grace_ == 0) {
        bool fan_need_publish = false;
        const ClimateFanMode new_fan_mode = XYEAdapter::get_climate_fan_mode(exr.target_fan_speed);
        if (!this->fan_mode.has_value() || this->fan_mode.value() != new_fan_mode) {
          this->fan_mode = new_fan_mode;
          fan_need_publish = true;
        }
        if (fan_need_publish)
          this->publish_state();
      }
      // Note: Previous versions validated fixed protocol marker bytes (0xBC, 0xD6, 0x80, 0x80, 0x80, 0x80)
      // but investigation shows these bytes are actually dynamic engineering values:
      // - Bytes 19-20 (0xBCD6): 16-bit compressor frequency or outdoor fan RPM
      // - Bytes 26-29 (0x80): Subsystem OK flags (compressor, outdoor fan, 4-way valve, inverter)
      // The validation has been removed to support all unit models correctly.
      ForceReadNextCycle = 0;
      break;
    }
    default:
      break;
  }
}

uint8_t ClimateMideaXYE::CalculateSetTime(uint32_t time) {
  uint32_t current_time = time;
  uint8_t timeValue = 0;

  if (0 < (current_time / 960)) {
    timeValue |= 0x40;
    current_time = current_time % 960;
  }
  if (0 < (current_time / 480)) {
    timeValue |= 0x20;
    current_time = current_time % 480;
  }
  if (0 < (current_time / 240)) {
    timeValue |= 0x10;
    current_time = current_time % 240;
  }
  if (0 < (current_time / 120)) {
    timeValue |= 0x08;
    current_time = current_time % 120;
  }
  if (0 < (current_time / 60)) {
    timeValue |= 0x04;
    current_time = current_time % 60;
  }
  if (0 < (current_time / 30)) {
    timeValue |= 0x02;
    current_time = current_time % 30;
  }
  if (0 < (current_time / 15)) {
    timeValue |= 0x01;
    current_time = current_time % 15;
  }
  return timeValue;
}

uint32_t ClimateMideaXYE::CalculateGetTime(uint8_t time) {
  uint32_t timeValue = 0;

  if (time & 0x40) {
    timeValue += 960;
  }
  if (time & 0x20) {
    timeValue += 480;
  }
  if (time & 0x10) {
    timeValue += 240;
  }
  if (time & 0x08) {
    timeValue += 120;
  }
  if (time & 0x04) {
    timeValue += 60;
  }
  if (time & 0x02) {
    timeValue += 30;
  }
  if (time & 0x01) {
    timeValue += 15;
  }
  return timeValue;
}

climate::ClimateTraits ClimateMideaXYE::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_visual_min_temperature(17);
  traits.set_visual_max_temperature(30);
  traits.set_visual_temperature_step(1.0);
  traits.set_visual_current_temperature_step(VISUAL_CURRENT_TEMPERATURE_STEP);
  traits.set_supported_modes(this->supported_modes_);
  traits.set_supported_swing_modes(this->supported_swing_modes_);
  traits.set_supported_presets(this->supported_presets_);
  /* + MINIMAL SET OF CAPABILITIES */
  traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
  traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
  traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
  traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_OFF);  // Can't set it but will be reported

  if (!traits.get_supported_modes().empty())
    traits.add_supported_mode(ClimateMode::CLIMATE_MODE_OFF);
  if (!traits.get_supported_swing_modes().empty())
    traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
  if (!traits.get_supported_presets().empty())
    traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);

  return traits;
}

void ClimateMideaXYE::dump_config() {
  ESP_LOGCONFIG(Constants::TAG, "MideaXYE:");
  ESP_LOGCONFIG(Constants::TAG, "  [x] Period: %dms", this->get_update_interval());
  ESP_LOGCONFIG(Constants::TAG, "  [x] Response timeout: %dms", this->response_timeout);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Runtime use Fahrenheit: %d", this->use_fahrenheit_);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Runtime temperature encoding: %s",
                this->raw_fahrenheit_temperatures_ ? "raw_fahrenheit" : "standard");
  ESP_LOGCONFIG(Constants::TAG, "  [x] Runtime target temperature source: %s",
                this->target_temperature_from_c0_ ? "C0" : "C4");
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen use Fahrenheit: %d", build_info::CONFIG_USE_FAHRENHEIT);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen temperature encoding: %s",
                build_info::CONFIG_TEMPERATURE_ENCODING);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen target temperature source: %s",
                build_info::CONFIG_TARGET_TEMPERATURE_SOURCE);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen raw Fahrenheit temperatures: %d",
                build_info::CONFIG_RAW_FAHRENHEIT_TEMPERATURES);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen target temperature from C0: %d",
                build_info::CONFIG_TARGET_TEMPERATURE_FROM_C0);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Codegen sync fan mode from device: %d",
                build_info::CONFIG_SYNC_FAN_MODE_FROM_DEVICE);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Source commit: %s%s", build_info::GIT_COMMIT,
                build_info::GIT_DIRTY ? " (dirty)" : "");
  ESP_LOGCONFIG(Constants::TAG, "  [x] Source branch: %s", build_info::GIT_BRANCH);
  ESP_LOGCONFIG(Constants::TAG, "  [x] Source remote: %s", build_info::GIT_REMOTE);

#ifdef USE_REMOTE_TRANSMITTER
  ESP_LOGCONFIG(Constants::TAG, "  [x] Using RemoteTransmitter");
#endif
  this->dump_traits_(Constants::TAG);
}

/* ACTIONS */

void ClimateMideaXYE::do_follow_me(float temperature, bool beeper) {
#ifdef USE_REMOTE_TRANSMITTER
  IrFollowMeData data(static_cast<uint8_t>(lroundf(temperature)), beeper);
  this->transmitter_.transmit(data);
#else
  // Prepare Follow-Me command for temperature update
  tx_data = TransmitData(Command::FOLLOW_ME);
  auto &d = tx_data.message.data.standard;

  // timer_stop is a subcommand type field for Follow-Me commands.
  // Subcommand values: 0x06=Init, 0x02=Update, 0x04=Static pressure
  // The followMeInit flag tracks whether we've sent the initialization command.
  // It gets reset to false whenever the AC mode changes (see control() function),
  // ensuring a proper initialization sequence after mode changes.
  d.timer_stop = followMeInit ? FOLLOWME_SUBCOMMAND_UPDATE : FOLLOWME_SUBCOMMAND_INIT;
  if (!followMeInit)
    followMeInit = true;
  lastFollowMeTemperature = static_cast<uint8_t>(lroundf(temperature));
  d.mode_flags = static_cast<ModeFlags>(lastFollowMeTemperature);
  tx_data.update_crc();
  // Only send if mode is something other than off.
  // Wired controller does not send Follow-Me command when off.
  if (this->mode != ClimateMode::CLIMATE_MODE_OFF) {
    if (controlState != ControlState::WAIT_DATA) {
      controlState = ControlState::SEND_FOLLOWME;
    } else {
      queuedCommand = ControlState::SEND_FOLLOWME;
    }
    ESP_LOGI(Constants::TAG, "Queued Follow-Me data.");
  }
#endif
}

void ClimateMideaXYE::set_static_pressure(uint8_t static_pressure) {
  if (static_pressure > 15) {
    ESP_LOGW(Constants::TAG, "Cannot set static pressure %d > 15", static_pressure);
    return;
  }

  // Prepare Follow-Me command for static pressure setting
  tx_data = TransmitData(Command::FOLLOW_ME);
  auto &d = tx_data.message.data.standard;
  d.target_temperature.value = static_cast<uint8_t>(STATIC_PRESSURE_FLAG | (static_pressure & STATIC_PRESSURE_VALUE_MASK));
  d.timer_stop = FOLLOWME_SUBCOMMAND_STOP;
  d.mode_flags = static_cast<ModeFlags>(lastFollowMeTemperature);
  tx_data.update_crc();

  if (this->mode == ClimateMode::CLIMATE_MODE_OFF) {
    if (controlState != ControlState::WAIT_DATA) {
      controlState = ControlState::SEND_FOLLOWME;
    } else {
      queuedCommand = ControlState::SEND_FOLLOWME;
    }
    ESP_LOGI(Constants::TAG, "Queued setting static pressure to %d", static_pressure);
  } else {
    ESP_LOGW(Constants::TAG, "Cannot set static pressure while unit is running");
  }
}

void ClimateMideaXYE::do_swing_step() {
#ifdef USE_REMOTE_TRANSMITTER
  IrSpecialData data(0x01);
  this->transmitter_.transmit(data);
#else
  ESP_LOGW(Constants::TAG, "Action needs remote_transmitter component");
#endif
}

void ClimateMideaXYE::do_display_toggle() {
#ifdef USE_REMOTE_TRANSMITTER
  IrSpecialData data(0x08);
  this->transmitter_.transmit(data);
#else
  ESP_LOGW(Constants::TAG, "Action needs remote_transmitter component");
#endif
}

void ClimateMideaXYE::on_follow_me_sensor_update_(float state) {
  if (std::isnan(state)) {
    return;
  }
  
  // Update current_temperature with the sensor value
  bool need_publish = false;
  this->update_current_temperature_from_sensors_(need_publish);
  if (need_publish) {
    this->publish_state();
  }

  // Send follow_me command with the sensor temperature
  this->do_follow_me(state, false);
}

void ClimateMideaXYE::update_current_temperature_from_sensors_(bool &need_publish) {
  // Use follow_me_sensor as current_temperature if available, otherwise use internal temperature
  if (this->follow_me_sensor_ != nullptr && this->follow_me_sensor_->has_state() &&
      !std::isnan(this->follow_me_sensor_->state)) {
    update_property(this->current_temperature, this->follow_me_sensor_->state, need_publish);
  } else if (!std::isnan(this->internal_temperature_)) {
    update_property(this->current_temperature, this->internal_temperature_, need_publish);
  }
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
