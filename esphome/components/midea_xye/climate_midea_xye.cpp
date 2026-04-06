#ifdef USE_ARDUINO

#include "climate_midea_xye.h"

#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

const char *const Constants::TAG = "midea_xye";
const char *const Constants::FREEZE_PROTECTION = "Freeze Protection";
const char *const Constants::SILENT = "Silent";
const char *const Constants::TURBO = "Turbo";

static void set_sensor(Sensor *sensor, float value) {
  if (sensor != nullptr && (!sensor->has_state() || sensor->get_raw_state() != value))
    sensor->publish_state(value);
}

static void set_number(number::Number *number, float value) {
  if (number != nullptr && (!number->has_state() || number->state != value))
    number->publish_state(value);
}

template<typename T> void update_property(T &property, const T &value, bool &flag) {
  if (property != value) {
    property = value;
    flag = true;
  }
}

// ---------------------------------------------------------------------------
// XYEAdapter – value conversions between XYE protocol types and ESPHome types
// ---------------------------------------------------------------------------

ClimateMode XYEAdapter::get_climate_mode(OperationMode op_mode) noexcept {
  ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
  switch (static_cast<uint8_t>(op_mode) & OP_MODE_VALUE_MASK) {
    case OP_MODE_OFF:  mode = ClimateMode::CLIMATE_MODE_OFF;       break;
    case OP_MODE_AUTO: mode = ClimateMode::CLIMATE_MODE_HEAT_COOL; break;
    case OP_MODE_FAN:  mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;  break;
    case OP_MODE_DRY:  mode = ClimateMode::CLIMATE_MODE_DRY;       break;
    case OP_MODE_HEAT: mode = ClimateMode::CLIMATE_MODE_HEAT;      break;
    case OP_MODE_COOL: mode = ClimateMode::CLIMATE_MODE_COOL;      break;
  }
  // AUTO_FLAG (0x10) overrides the base value: if set while not already OFF,
  // the unit is in heat/cool (auto) mode.
  if (mode != ClimateMode::CLIMATE_MODE_OFF &&
      (static_cast<uint8_t>(op_mode) & OP_MODE_AUTO_FLAG) == OP_MODE_AUTO_FLAG) {
    mode = ClimateMode::CLIMATE_MODE_HEAT_COOL;
  }
  return mode;
}

ClimateFanMode XYEAdapter::get_climate_fan_mode(FanMode fan_mode) noexcept {
  // The AUTO flag (bit 7, 0x80) takes precedence over the speed nibble.
  if ((static_cast<uint8_t>(fan_mode) & FAN_MODE_AUTO) == FAN_MODE_AUTO)
    return ClimateFanMode::CLIMATE_FAN_AUTO;
  switch (static_cast<uint8_t>(fan_mode) & FAN_SPEED_MASK) {
    case FAN_MODE_HIGH:   return ClimateFanMode::CLIMATE_FAN_HIGH;
    case FAN_MODE_MEDIUM: return ClimateFanMode::CLIMATE_FAN_MEDIUM;
    case FAN_MODE_LOW:    return ClimateFanMode::CLIMATE_FAN_LOW;
    case FAN_MODE_OFF:    return ClimateFanMode::CLIMATE_FAN_OFF;
    default:              return ClimateFanMode::CLIMATE_FAN_AUTO;
  }
}

float XYEAdapter::get_temperature(uint8_t raw) noexcept { return (raw - 0x28) / 2.0f; }

float XYEAdapter::get_target_temperature(uint8_t raw) noexcept {
  return static_cast<float>(raw & SET_TEMP_VALUE_MASK);
}

ClimateAction XYEAdapter::get_climate_action(ClimateMode mode, FanMode fan_mode,
                                              OperationMode op_mode) noexcept {
  const bool fan_running = (static_cast<uint8_t>(fan_mode) & FAN_SPEED_MASK) != 0x00;

  ClimateAction action = ClimateAction::CLIMATE_ACTION_IDLE;
  if (mode == ClimateMode::CLIMATE_MODE_HEAT && fan_running) {
    action = ClimateAction::CLIMATE_ACTION_HEATING;
  } else if (mode == ClimateMode::CLIMATE_MODE_COOL && fan_running) {
    action = ClimateAction::CLIMATE_ACTION_COOLING;
  }

  // In heat/cool (auto) mode refine the action using the unit's reported sub-mode.
  if (mode == ClimateMode::CLIMATE_MODE_HEAT_COOL) {
    const uint8_t op_raw = static_cast<uint8_t>(op_mode) & OP_MODE_VALUE_MASK;
    if (op_raw == OP_MODE_COOL)
      action = ClimateAction::CLIMATE_ACTION_COOLING;
    else if (op_raw == OP_MODE_FAN)
      action = ClimateAction::CLIMATE_ACTION_FAN;
    else if (op_raw == OP_MODE_HEAT)
      action = ClimateAction::CLIMATE_ACTION_HEATING;
  }

  return action;
}

OperationMode XYEAdapter::get_operation_mode(ClimateMode mode) noexcept {
  switch (mode) {
    case ClimateMode::CLIMATE_MODE_HEAT_COOL: return OperationMode::AUTO;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:  return OperationMode::FAN;
    case ClimateMode::CLIMATE_MODE_DRY:       return OperationMode::DRY;
    case ClimateMode::CLIMATE_MODE_HEAT:      return OperationMode::HEAT;
    case ClimateMode::CLIMATE_MODE_COOL:      return OperationMode::COOL;
    default:                                  return OperationMode::OFF;
  }
}

FanMode XYEAdapter::get_fan_mode(ClimateFanMode fan_mode) noexcept {
  switch (fan_mode) {
    case ClimateFanMode::CLIMATE_FAN_HIGH:   return FanMode::FAN_HIGH;
    case ClimateFanMode::CLIMATE_FAN_MEDIUM: return FanMode::FAN_MEDIUM;
    case ClimateFanMode::CLIMATE_FAN_LOW:    return FanMode::FAN_LOW;
    default:                                 return FanMode::FAN_AUTO;
  }
}

// ---------------------------------------------------------------------------

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

void ClimateMideaXYE::setACParams() {
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

  // Data always comes in as C, but user may want it set in F.
  if (this->use_fahrenheit_) {
    float tgt_temp = ((9.0 / 5.0) * this->target_temperature + 32.0);
    d.target_temperature.value = static_cast<uint8_t>(static_cast<int>(tgt_temp) + FAHRENHEIT_TEMP_OFFSET);
  } else {
    d.target_temperature.value = static_cast<uint8_t>(static_cast<int>(this->target_temperature));
  }

  d.mode_flags = static_cast<ModeFlags>(
      ((this->preset == ClimatePreset::CLIMATE_PRESET_BOOST) * MODE_FLAG_AUX_HEAT) |
      ((this->preset == ClimatePreset::CLIMATE_PRESET_SLEEP) * MODE_FLAG_ECO) |
      ((this->swing_mode != ClimateSwingMode::CLIMATE_SWING_OFF) * MODE_FLAG_SWING) | (0 * MODE_FLAG_VENT));

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
      rx_data.print_debug(i, Constants::TAG, ESPHOME_LOG_LEVEL_DEBUG);
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
      setACParams();
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

  const auto &qr = rx_data.message.data.query_response;

  switch (rx_data.message.frame.header.command) {
    case Command::QUERY: {
      ClimatePreset preset = ClimatePreset::CLIMATE_PRESET_NONE;

      const ClimateMode mode = XYEAdapter::get_climate_mode(qr.operation_mode);
      const ClimateFanMode fan_mode = XYEAdapter::get_climate_fan_mode(qr.fan_mode);

      if (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_AUX_HEAT)
        preset = ClimatePreset::CLIMATE_PRESET_BOOST;
      else if (static_cast<uint8_t>(qr.mode_flags) & MODE_FLAG_ECO)
        preset = ClimatePreset::CLIMATE_PRESET_SLEEP;

      bool need_publish = false;

      update_property(this->mode, mode, need_publish);
      if (mode != ClimateMode::CLIMATE_MODE_OFF) {
        this->last_on_mode_ = mode;
      }

      if (mode != ClimateMode::CLIMATE_MODE_OFF || ForceReadNextCycle == 1) {
        // Store the internal temperature from the XYE bus
        this->internal_temperature_ = XYEAdapter::get_temperature(qr.t1_temperature.value);

        // Publish the internal temperature to the sensor if configured
        set_sensor(this->internal_current_temperature_sensor_, this->internal_temperature_);

        // Update current_temperature based on sensor availability
        this->update_current_temperature_from_sensors_(need_publish);

#ifndef SET_TARGET_TEMP_ON_QUERY
        // Temperature is raw Celsius; bit 6 (SET_TEMP_STATUS_FLAG / 0x40) may be set
        // by the unit in certain states and must be masked out before use.
        update_property(this->target_temperature,
                        XYEAdapter::get_target_temperature(qr.target_temperature.value), need_publish);
#endif

        update_property(this->action,
                        XYEAdapter::get_climate_action(mode, qr.fan_mode, qr.operation_mode),
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

      set_sensor(this->temperature_2a_sensor_, XYEAdapter::get_temperature(qr.t2a_temperature.value));
      set_sensor(this->temperature_2b_sensor_, XYEAdapter::get_temperature(qr.t2b_temperature.value));
      set_sensor(this->temperature_3_sensor_, XYEAdapter::get_temperature(qr.t3_temperature.value));
      set_sensor(this->current_sensor_, static_cast<float>(qr.current));
      set_sensor(this->timer_start_sensor_, CalculateGetTime(qr.timer_start));
      set_sensor(this->timer_stop_sensor_, CalculateGetTime(qr.timer_stop));
      set_sensor(this->error_flags_sensor_, static_cast<float>(qr.error_flags.value()));
      set_sensor(this->protect_flags_sensor_, static_cast<float>(qr.protect_flags.value()));
      break;
    }
    case Command::QUERY_EXTENDED: {
      bool need_publish = false;
      const auto &exr = rx_data.message.data.extended_query_response;
      set_sensor(this->outdoor_sensor_, XYEAdapter::get_temperature(exr.outdoor_temperature.value));
      set_number(this->static_pressure_number_, static_cast<float>(STATIC_PRESSURE_VALUE_MASK & exr.static_pressure));
#ifdef SET_TARGET_TEMP_ON_EXTENDED_QUERY
      if (this->mode != ClimateMode::CLIMATE_MODE_OFF || ForceReadNextCycle == 1) {
        float incoming_target_temp = 0.0;
        if (this->use_fahrenheit_) {
          incoming_target_temp = (float) (((exr.target_temperature.value - FAHRENHEIT_TEMP_OFFSET) - 32.0) * 5.0 / 9.0);
          if (incoming_target_temp != this->target_temperature) {
            need_publish = true;
            update_property(this->target_temperature, incoming_target_temp, need_publish);
          }
        } else {
          incoming_target_temp = XYEAdapter::get_temperature(exr.target_temperature.value);
          if (incoming_target_temp != this->target_temperature) {
            need_publish = true;
            update_property(this->target_temperature, incoming_target_temp, need_publish);
          }
        }
        if (need_publish)
          this->publish_state();
      }
#endif
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
  traits.set_supported_modes(this->supported_modes_);
  traits.set_supported_swing_modes(this->supported_swing_modes_);
  traits.set_supported_presets(this->supported_presets_);
  traits.set_supported_custom_presets(this->supported_custom_presets_);
  traits.set_supported_custom_fan_modes(this->supported_custom_fan_modes_);
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
  ESP_LOGCONFIG(Constants::TAG, "  [x] Use Fahrenheit: %d", this->use_fahrenheit_);

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
  d.timer_stop = FOLLOWME_SUBCOMMAND_STATIC_PRESSURE;
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
