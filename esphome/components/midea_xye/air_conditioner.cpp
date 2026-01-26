#ifdef USE_ARDUINO

#include "air_conditioner.h"

#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace ac {

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

void AirConditioner::control(const ClimateCall &call) {
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

  if (controlState != STATE_WAIT_DATA) {
    controlState = STATE_SEND_SET;
  } else {
    queuedCommand = STATE_SEND_SET;
  }
}

void AirConditioner::setup() {
  // this->uart_->check_uart_settings(4800, 1, UART_CONFIG_PARITY_NONE, 8);
  this->last_on_mode_ = *this->supported_modes_.begin();
  controlState = STATE_SEND_QUERY;
  ForceReadNextCycle = 1;
  followMeInit = false;
  last_follow_me_update_ = 0;

  // Start up in Auto fan mode (since unit doesn't report it correctly)
  this->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
}

void AirConditioner::set_follow_me_sensor(Sensor *sensor) {
  this->follow_me_sensor_ = sensor;
  if (sensor != nullptr) {
    sensor->add_on_state_callback([this](float state) { this->on_follow_me_sensor_update_(state); });
  }
}

// TODO: Not sure if we really need this.
void AirConditioner::setPowerState(bool state) {
  if (state)
    this->mode = this->last_on_mode_;
  else
    this->mode = ClimateMode::CLIMATE_MODE_OFF;

  if (controlState != STATE_WAIT_DATA) {
    controlState = STATE_SEND_SET;
  } else {
    queuedCommand = STATE_SEND_SET;
  }
}

void AirConditioner::prepareTXData(uint8_t command) {
  TXData[0] = PREAMBLE;
  TXData[1] = command;
  TXData[2] = SERVER_ID;
  TXData[3] = CLIENT_ID;
  TXData[4] = FROM_CLIENT;
  TXData[5] = CLIENT_ID;
  TXData[6] = 0;
  TXData[7] = 0;
  TXData[8] = 0;
  TXData[9] = 0;
  TXData[10] = 0;
  TXData[11] = 0;
  TXData[12] = 0;
  TXData[13] = 0xFF - TXData[1];
  TXData[15] = PROLOGUE;
  TXData[14] = CalculateCRC(TXData, TX_LEN);
}

void AirConditioner::setACParams() {
  // construct set command
  prepareTXData(CLIENT_COMMAND_SET);

  // set mode
  switch (this->mode) {
    case ClimateMode::CLIMATE_MODE_OFF:
      TXData[6] = OP_MODE_OFF;
      break;
    case ClimateMode::CLIMATE_MODE_HEAT_COOL:
      TXData[6] = OP_MODE_AUTO;
      break;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:
      TXData[6] = OP_MODE_FAN;
      break;
    case ClimateMode::CLIMATE_MODE_DRY:
      TXData[6] = OP_MODE_DRY;
      break;
    case ClimateMode::CLIMATE_MODE_HEAT:
      TXData[6] = OP_MODE_HEAT;
      break;
    case ClimateMode::CLIMATE_MODE_COOL:
      TXData[6] = OP_MODE_COOL;
      break;
    default:
      TXData[6] = OP_MODE_OFF;
  }
  // set fan mode
  if (this->mode != ClimateMode::CLIMATE_MODE_HEAT_COOL) {
    switch (this->fan_mode.value()) {
      case ClimateFanMode::CLIMATE_FAN_AUTO:
        TXData[7] = FAN_MODE_AUTO;
        break;
      case ClimateFanMode::CLIMATE_FAN_HIGH:
        TXData[7] = FAN_MODE_HIGH;
        break;
      case ClimateFanMode::CLIMATE_FAN_MEDIUM:
        TXData[7] = FAN_MODE_MEDIUM;
        break;
      case ClimateFanMode::CLIMATE_FAN_LOW:
        TXData[7] = FAN_MODE_LOW;
        break;
      default:
        TXData[7] = FAN_MODE_AUTO;
    }
  } else {
    // Auto is full-auto - can't set fan mode either.
    this->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
    TXData[7] = FAN_MODE_AUTO;
  }
  // set temp
  // Data always comes in as C, but user may want it set in F.
  if (this->use_fahrenheit_) {
    float tgt_temp = ((9.0 / 5.0) * this->target_temperature + 32.0);

    TXData[8] = (int) tgt_temp + 0x87;  // Offset from actual to engineering value
  } else {
    TXData[8] = (int) this->target_temperature;
  }

  // set mode flags
  TXData[11] = ((this->preset == ClimatePreset::CLIMATE_PRESET_BOOST) * MODE_FLAG_AUX_HEAT) |
               ((this->preset == ClimatePreset::CLIMATE_PRESET_SLEEP) * MODE_FLAG_ECO) |
               ((this->swing_mode != ClimateSwingMode::CLIMATE_SWING_OFF) * MODE_FLAG_SWING) | (0 * MODE_FLAG_VENT);

  // set timer start
  // TODO: This is not tested. If you use it probably want to switch to
  // State.TimerStart so timer doesn't get ovedrridden TXData[9] =
  // CalculateSetTime(DesiredState.TimerStart); set timer stop TXData[10] =
  // CalculateSetTime(DesiredState.TimerStop);

  TXData[14] = CalculateCRC(TXData, TX_LEN);
}

void AirConditioner::sendRecv(uint8_t cmdSent) {
  // TODO: Reimplement flow control for manual RS485 flow control chips
  // digitalWrite(ComControlPin, RS485_TX_PIN_VALUE);
  this->uart_->write_array(TXData, TX_LEN);
  this->uart_->flush();
  controlState = STATE_WAIT_DATA;
  // Delay the remaining for 100 ms to allow response from the AC unit.
  this->set_timeout("read-result", 100, [this, cmdSent]() {
    // digitalWrite(ComControlPin, RS485_RX_PIN_VALUE);

    uint8_t i = 0;
    while (this->uart_->available()) {
      if (i < RX_LEN)
        this->uart_->read_byte(&RXData[i]);
      i++;
    }
    if (i == RX_LEN) {
      if (cmdSent != CLIENT_COMMAND_SET) {
        ParseResponse(cmdSent);
      }
      if (queuedCommand != 0) {
        controlState = queuedCommand;
        queuedCommand = 0;
      } else {
        switch (cmdSent) {
          case CLIENT_COMMAND_QUERY:
            controlState = STATE_SEND_QUERY_EXTENDED;
            break;
          case CLIENT_COMMAND_SET:
            controlState = STATE_SEND_FOLLOWME;
            break;
          case CLIENT_COMMAND_QUERY_EXTENDED:
            controlState = STATE_SEND_QUERY;
            break;
          case CLIENT_COMMAND_FOLLOWME:
            controlState = STATE_SEND_QUERY;
            break;
        }
      }
    } else {
      ESP_LOGE(Constants::TAG, "Received incorrect message length from AC for Command %02X", cmdSent);
    }
  });
}

void AirConditioner::update() {
  uint8_t cmdSent = 0x00;
  // Possible States:
  // 0: Waiting for Response from Command
  // 1: Sending Set C3 Command
  // 2: Sending Set C6 Command
  // 3: Sending Query C0 Command
  // 4: Sending Query C4 Command
  switch (controlState) {
    case STATE_SEND_SET: {
      setACParams();
      cmdSent = CLIENT_COMMAND_SET;
      sendRecv(cmdSent);
      break;
    }
    case STATE_SEND_FOLLOWME: {
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
    case STATE_SEND_QUERY: {
      // construct query command
      prepareTXData(CLIENT_COMMAND_QUERY);
      cmdSent = CLIENT_COMMAND_QUERY;
      sendRecv(cmdSent);
      break;
    }
    case STATE_SEND_QUERY_EXTENDED: {
      prepareTXData(CLIENT_COMMAND_QUERY_EXTENDED);
      cmdSent = CLIENT_COMMAND_QUERY_EXTENDED;
      sendRecv(cmdSent);
      break;
    }
    case STATE_WAIT_DATA: {
      // Wait for data to processed. Do nothing during the loop.
      break;
    }
  }
  
  // Periodically update follow_me every 30 seconds if sensor is configured
  if (this->follow_me_sensor_ != nullptr && this->follow_me_sensor_->has_state()) {
    uint32_t now = millis();
    // Update every 30 seconds (30000ms)
    // This handles millis() overflow correctly using unsigned arithmetic
    if (now - this->last_follow_me_update_ >= 30000) {
      this->update_follow_me_();
    }
  }
}

uint8_t AirConditioner::CalculateCRC(uint8_t *data, uint8_t len) {
  uint32_t crc = 0;
  for (uint8_t i = 0; i < len; i++) {
    if (i != len - 2) {
      crc += data[i];
    }
  }
  return 0xFF - (crc & 0xFF);
}

void AirConditioner::ParseResponse(uint8_t cmdSent) {
  // validate the response
  if ((RXData[RX_BYTE_PREAMBLE] == PREAMBLE) && (RXData[RX_BYTE_PROLOGUE] == PROLOGUE) &&
      (RXData[RX_BYTE_TO_CLIENT] == TO_CLIENT) && (RXData[RX_BYTE_CRC] == CalculateCRC(RXData, RX_LEN))) {
    switch (RXData[RX_BYTE_COMMAND_TYPE]) {
      case CLIENT_COMMAND_QUERY: {
        ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
        ClimateFanMode fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
        ClimatePreset preset = ClimatePreset::CLIMATE_PRESET_NONE;

        switch (RXData[RX_C0_BYTE_OP_MODE] & 0xEF) {
          case OP_MODE_OFF:
            mode = ClimateMode::CLIMATE_MODE_OFF;
            break;
          case OP_MODE_AUTO:
            mode = ClimateMode::CLIMATE_MODE_HEAT_COOL;
            break;
          case OP_MODE_FAN:
            mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
            break;
          case OP_MODE_DRY:
            mode = ClimateMode::CLIMATE_MODE_DRY;
            break;
          case OP_MODE_HEAT:
            mode = ClimateMode::CLIMATE_MODE_HEAT;
            break;
          case OP_MODE_COOL:
            mode = ClimateMode::CLIMATE_MODE_COOL;
            break;
        }

        // The unit seems to show 0x10 when off after running auto.
        // Check to see if we haven't already matched to OFF state.
        // If not, and we match otherwise, we are in auto mode.
        if (mode != ClimateMode::CLIMATE_MODE_OFF &&
            ((RXData[RX_C0_BYTE_OP_MODE] & OP_MODE_AUTO_FLAG) == OP_MODE_AUTO_FLAG)) {
          mode = ClimateMode::CLIMATE_MODE_HEAT_COOL;
        }

        uint8_t current_fan_speed = RXData[RX_C0_BYTE_FAN_MODE] & 0x0F;
        switch (current_fan_speed) {
          case FAN_MODE_HIGH:
            fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
            break;
          case FAN_MODE_MEDIUM:
            fan_mode = ClimateFanMode::CLIMATE_FAN_MEDIUM;
            break;
          case FAN_MODE_LOW:
            fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
            break;
          case FAN_MODE_OFF:
            fan_mode = ClimateFanMode::CLIMATE_FAN_OFF;
            break;
        }
        if ((RXData[RX_C0_BYTE_FAN_MODE] & FAN_MODE_AUTO) == FAN_MODE_AUTO) {
          fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
        }

        if (RXData[RX_C0_BYTE_MODE_FLAGS] & MODE_FLAG_AUX_HEAT)
          preset = ClimatePreset::CLIMATE_PRESET_BOOST;
        else if (RXData[RX_C0_BYTE_MODE_FLAGS] & MODE_FLAG_ECO)
          preset = ClimatePreset::CLIMATE_PRESET_SLEEP;

        bool need_publish = false;

        update_property(this->mode, mode, need_publish);
        if (mode != ClimateMode::CLIMATE_MODE_OFF)  // Don't update below states
                                                    // unless mode is an ON state
        {
          this->last_on_mode_ = mode;
        }

        if (mode != ClimateMode::CLIMATE_MODE_OFF ||
            ForceReadNextCycle == 1)  // Don't update below states unless mode is an ON state
        {
          // Don't update the fan mode. Assume it set correctly.
          // Show Heating vs Heat at least in Heat mode. Will figure
          // out how to determine if compressor is on in other modes later.
          // Store the internal temperature from the XYE bus
          this->internal_temperature_ = CalculateTemp(RXData[RX_C0_BYTE_T1_TEMP]);

          // Publish the internal temperature to the sensor if configured
          set_sensor(this->internal_current_temperature_sensor_, this->internal_temperature_);

          // Update current_temperature based on sensor availability
          this->update_current_temperature_from_sensors_(need_publish);

#ifndef SET_TARGET_TEMP_ON_QUERY
          // Target temperature always comes in as C, but user may want it in F.
          update_property(this->target_temperature, static_cast<float>(RXData[RX_C0_BYTE_SET_TEMP]), need_publish);
#endif

          if ((this->mode == climate::CLIMATE_MODE_HEAT) && (RXData[9] & 0x0F) != 0x00) {
            this->action = climate::CLIMATE_ACTION_HEATING;
            need_publish = true;
          } else if ((this->action != climate::CLIMATE_ACTION_IDLE) && (RXData[9] & 0x0F) == 0x00) {
            this->action = climate::CLIMATE_ACTION_IDLE;
            need_publish = true;
          }

          if ((this->mode == climate::CLIMATE_MODE_HEAT_COOL) &&
              ((RXData[RX_C0_BYTE_OP_MODE] & 0xEF) == OP_MODE_COOL) &&
              (this->action != climate::CLIMATE_ACTION_COOLING)) {
            this->action = climate::CLIMATE_ACTION_COOLING;
            need_publish = true;
          } else if ((this->mode == climate::CLIMATE_MODE_HEAT_COOL) &&
                     ((RXData[RX_C0_BYTE_OP_MODE] & 0xEF) == OP_MODE_FAN) &&
                     (this->action != climate::CLIMATE_ACTION_FAN)) {
            this->action = climate::CLIMATE_ACTION_FAN;
            need_publish = true;
          } else if ((this->mode == climate::CLIMATE_MODE_HEAT_COOL) &&
                     ((RXData[RX_C0_BYTE_OP_MODE] & 0xEF) == OP_MODE_HEAT) &&
                     (this->action != climate::CLIMATE_ACTION_HEATING)) {
            this->action = climate::CLIMATE_ACTION_HEATING;
            need_publish = true;
          }

          if ((this->swing_mode != ClimateSwingMode::CLIMATE_SWING_OFF) !=
              (bool) (RXData[RX_C0_BYTE_MODE_FLAGS] & MODE_FLAG_SWING))
            need_publish = true;
          this->swing_mode = (RXData[RX_C0_BYTE_MODE_FLAGS] & MODE_FLAG_SWING)
                                 ? ClimateSwingMode::CLIMATE_SWING_VERTICAL
                                 : ClimateSwingMode::CLIMATE_SWING_OFF;
          if (this->preset != preset)
            need_publish = true;
          this->preset = preset;
        } else if ((this->action != climate::CLIMATE_ACTION_IDLE) && (RXData[9] & 0x0F) == 0x00) {
          this->action = climate::CLIMATE_ACTION_IDLE;
          need_publish = true;
        }

        if (need_publish)
          this->publish_state();

        set_sensor(this->temperature_2a_sensor_, CalculateTemp(RXData[RX_C0_BYTE_T2A_TEMP]));
        set_sensor(this->temperature_2b_sensor_, CalculateTemp(RXData[RX_C0_BYTE_T2B_TEMP]));
        set_sensor(this->temperature_3_sensor_, CalculateTemp(RXData[RX_C0_BYTE_T3_TEMP]));
        set_sensor(this->current_sensor_, RXData[RX_C0_BYTE_CURRENT]);
        set_sensor(this->timer_start_sensor_, CalculateGetTime(RXData[RX_C0_BYTE_TIMER_START]));
        set_sensor(this->timer_stop_sensor_, CalculateGetTime(RXData[RX_C0_BYTE_TIMER_STOP]));
        set_sensor(this->error_flags_sensor_,
                   (RXData[RX_C0_BYTE_ERROR_FLAGS1] << 0) | (RXData[RX_C0_BYTE_ERROR_FLAGS2] << 8));
        set_sensor(this->protect_flags_sensor_,
                   (RXData[RX_C0_BYTE_PROTECT_FLAGS1] << 0) | (RXData[RX_C0_BYTE_PROTECT_FLAGS2] << 8));
        break;
      }
      case CLIENT_COMMAND_QUERY_EXTENDED:
        bool need_publish = false;
        set_sensor(this->outdoor_sensor_, CalculateTemp(RXData[21]));
        set_number(this->static_pressure_number_, 0x0F & RXData[24]);
#ifdef SET_TARGET_TEMP_ON_EXTENDED_QUERY
        if (mode != ClimateMode::CLIMATE_MODE_OFF ||
            ForceReadNextCycle == 1)  // Don't update below states unless mode is an ON state
        {
          float incoming_target_temp = 0.0;
          if (this->use_fahrenheit_) {
            incoming_target_temp = (float) (((RXData[RX_C4_BYTE_SET_TEMP] - 0x87) - 32.0) * 5.0 / 9.0);
            if (incoming_target_temp != this->target_temperature) {
              need_publish = true;
              update_property(this->target_temperature, incoming_target_temp, need_publish);
            }
          } else {
            incoming_target_temp = CalculateTemp(RXData[RX_C4_BYTE_SET_TEMP]);
            if (incoming_target_temp != this->target_temperature) {
              need_publish = true;
              update_property(this->target_temperature, incoming_target_temp, need_publish);
            }
          }
          if (need_publish)
            this->publish_state();
        }
#endif

        if (RXData[9] != 0x30 || RXData[10] != 0x98 || RXData[11] != 0x00 || RXData[12] != 0x00 || RXData[13] != 0x00 ||
            RXData[14] != 0x01 || RXData[15] != 0x20 || RXData[19] != 0xBC || RXData[20] != 0xD6 ||
            RXData[22] != 0x00 || RXData[23] != 0x00 || RXData[24] != 0xFF || RXData[25] != 0x00 ||
            RXData[26] != 0x80 || RXData[27] != 0x80 || RXData[28] != 0x80 || RXData[29] != 0x80) {
          ESP_LOGI(Constants::TAG,
                   "DEBUG C4: "
                   "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%"
                   "02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:"
                   "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
                   RXData[0], RXData[1], RXData[2], RXData[3], RXData[4], RXData[5], RXData[6], RXData[7], RXData[8],
                   RXData[9], RXData[10], RXData[11], RXData[12], RXData[13], RXData[14], RXData[15], RXData[16],
                   RXData[17], RXData[18], RXData[19], RXData[20], RXData[21], RXData[22], RXData[23], RXData[24],
                   RXData[25], RXData[26], RXData[27], RXData[28], RXData[29], RXData[30], RXData[31]);
        }
        ForceReadNextCycle = 0;
        break;
    }
  } else {
    ESP_LOGE(Constants::TAG, "Received invalid response from AC");
  }
}

uint8_t AirConditioner::CalculateSetTime(uint32_t time) {
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

uint32_t AirConditioner::CalculateGetTime(uint8_t time) {
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

float AirConditioner::CalculateTemp(uint8_t byte) { return (byte - 0x28) / 2.0; }

climate::ClimateTraits AirConditioner::traits() {
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

void AirConditioner::dump_config() {
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

void AirConditioner::do_follow_me(float temperature, bool beeper) {
#ifdef USE_REMOTE_TRANSMITTER
  IrFollowMeData data(static_cast<uint8_t>(lroundf(temperature)), beeper);
  this->transmitter_.transmit(data);
#else
  // Prepare Follow-Me command for temperature update
  prepareTXData(CLIENT_COMMAND_FOLLOWME);
  
  // TXData[10] is a subcommand type field for Follow-Me commands.
  // Subcommand values: 0x06=Init, 0x02=Update, 0x04=Static pressure
  // The followMeInit flag tracks whether we've sent the initialization command.
  // It gets reset to false whenever the AC mode changes (see control() function),
  // ensuring a proper initialization sequence after mode changes.
  if (followMeInit) {
    TXData[10] = FOLLOWME_SUBCOMMAND_UPDATE;  // Follow-Me update
  } else {
    TXData[10] = FOLLOWME_SUBCOMMAND_INIT;  // Follow-Me initialization
    followMeInit = true;
  }
  lastFollowMeTemperature = static_cast<uint8_t>(lroundf(temperature));
  TXData[11] = lastFollowMeTemperature;
  TXData[14] = CalculateCRC(TXData, TX_LEN);
  // Only send if mode is something other than off.
  // Wired controller does not send Follow-Me command when off.
  if (this->mode != ClimateMode::CLIMATE_MODE_OFF) {
    if (controlState != STATE_WAIT_DATA) {
      controlState = STATE_SEND_FOLLOWME;
    } else {
      queuedCommand = STATE_SEND_FOLLOWME;
    }
    ESP_LOGI(Constants::TAG, "Queued Follow-Me data.");
  }
#endif
}

void AirConditioner::set_static_pressure(uint8_t static_pressure) {
  if (static_pressure > 15) {
    ESP_LOGW(Constants::TAG, "Cannot set static pressure %d > 15", static_pressure);
    return;
  }

  // Prepare Follow-Me command for static pressure setting
  prepareTXData(CLIENT_COMMAND_FOLLOWME);
  TXData[8] = 0x10 | (static_pressure & 0x0F);
  TXData[10] = FOLLOWME_SUBCOMMAND_STATIC_PRESSURE;  // Subcommand type: Static pressure setting
  TXData[11] = lastFollowMeTemperature;
  TXData[14] = CalculateCRC(TXData, TX_LEN);

  if (this->mode == ClimateMode::CLIMATE_MODE_OFF) {
    if (controlState != STATE_WAIT_DATA) {
      controlState = STATE_SEND_FOLLOWME;
    } else {
      queuedCommand = STATE_SEND_FOLLOWME;
    }
    ESP_LOGI(Constants::TAG, "Queued setting static pressure to %d", static_pressure);
  } else {
    ESP_LOGW(Constants::TAG, "Cannot set static pressure while unit is running");
  }
}

void AirConditioner::do_swing_step() {
#ifdef USE_REMOTE_TRANSMITTER
  IrSpecialData data(0x01);
  this->transmitter_.transmit(data);
#else
  ESP_LOGW(Constants::TAG, "Action needs remote_transmitter component");
#endif
}

void AirConditioner::do_display_toggle() {
#ifdef USE_REMOTE_TRANSMITTER
  IrSpecialData data(0x08);
  this->transmitter_.transmit(data);
#else
  ESP_LOGW(Constants::TAG, "Action needs remote_transmitter component");
#endif
}

void AirConditioner::on_follow_me_sensor_update_(float state) {
  if (std::isnan(state)) {
    return;
  }
  
  // Update current_temperature with the sensor value
  bool need_publish = false;
  this->update_current_temperature_from_sensors_(need_publish);
  if (need_publish) {
    this->publish_state();
  }

  // Check if enough time has passed since last update (minimum 5 seconds to avoid excessive updates)
  uint32_t now = millis();
  // Handle millis() overflow correctly using unsigned arithmetic
  if (now - this->last_follow_me_update_ >= 5000) {
    this->update_follow_me_();
  }
}

void AirConditioner::update_follow_me_() {
  if (this->follow_me_sensor_ == nullptr || !this->follow_me_sensor_->has_state()) {
    return;
  }
  
  float temperature = this->follow_me_sensor_->state;
  if (std::isnan(temperature)) {
    return;
  }
  
  // Call the existing do_follow_me method with the sensor temperature
  this->do_follow_me(temperature, false);
  this->last_follow_me_update_ = millis();
}

void AirConditioner::update_current_temperature_from_sensors_(bool &need_publish) {
  // Use follow_me_sensor as current_temperature if available, otherwise use internal temperature
  if (this->follow_me_sensor_ != nullptr && this->follow_me_sensor_->has_state() &&
      !std::isnan(this->follow_me_sensor_->state)) {
    update_property(this->current_temperature, this->follow_me_sensor_->state, need_publish);
  } else if (!std::isnan(this->internal_temperature_)) {
    update_property(this->current_temperature, this->internal_temperature_, need_publish);
  }
}

}  // namespace ac
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
