#include "cn40_climate.h"
#include "esphome/core/log.h"

//static const char *const TAG = "cn40";
static const char *const TAG = "main";

namespace cn40 {

CN40Climate::CN40Climate(esphome::uart::UARTComponent *parent)
    : UARTDevice(parent) {
  ESP_LOGI(TAG, "CN40Climate constructor called");
}

void CN40Climate::setup() {
    ESP_LOGD(TAG, "CN40Climate setup starting.");
}

void CN40Climate::diag() {
    ESP_LOGD(TAG, "CN40Climate diag.");

    // Log RS485 direction pin (critical for M5 RS485 Base)
    ESP_LOGI(TAG, "RS485 DE/RE pin state: %d", gpio_get_level(GPIO_NUM_33));

    // Check for bus activity after 500ms
    this->set_timeout("uart_check", 500, [this]() {
        if (this->available()) {
            ESP_LOGI("cn40", "UART receiving data at boot.");
        } else {
            ESP_LOGW("cn40", "No UART data detected 500ms after boot.");
        }
    });

    // Try to read first byte after 2 seconds
    this->set_timeout("first_byte", 2000, [this]() {
        if (this->available()) {
            uint8_t b = this->read();
            ESP_LOGI(TAG, "First byte seen: 0x%02X", b);
         } else {
            ESP_LOGW(TAG, "Still no bytes after 2 seconds.");
         }
     });
}

void CN40Climate::loop() {
  while (available()) {

    ESP_LOGI(TAG, "UART available: %d bytes", this->available());

    uint8_t b = read();
    buffer_.push_back(b);

    ESP_LOGD(TAG, "RX: 0x%02X", b);

    // TODO: detect CN40 frame boundaries
    // TODO: validate header
    // TODO: validate CRC
    // TODO: parse payload
  }

  //diag();
}

esphome::climate::ClimateTraits CN40Climate::traits() {
  auto traits = esphome::climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supported_modes({
      esphome::climate::CLIMATE_MODE_OFF,
      esphome::climate::CLIMATE_MODE_COOL,
      esphome::climate::CLIMATE_MODE_HEAT,
      esphome::climate::CLIMATE_MODE_HEAT_COOL,
      esphome::climate::CLIMATE_MODE_DRY,
      esphome::climate::CLIMATE_MODE_FAN_ONLY,
  });
  traits.set_supported_fan_modes({
      esphome::climate::CLIMATE_FAN_AUTO,
      esphome::climate::CLIMATE_FAN_LOW,
      esphome::climate::CLIMATE_FAN_MEDIUM,
      esphome::climate::CLIMATE_FAN_HIGH,
  });
  return traits;
}

void CN40Climate::control(const esphome::climate::ClimateCall &call) {
  ESP_LOGD(TAG, "CN40Climate control.");
  if (call.get_mode().has_value())
    this->mode = *call.get_mode();

  if (call.get_target_temperature().has_value())
    this->target_temperature = *call.get_target_temperature();

  if (call.get_fan_mode().has_value())
    this->fan_mode = *call.get_fan_mode();

  send_command_();

  publish_state();
}

void CN40Climate::send_command_() {
  ESP_LOGD(TAG, "CN40Climate send command.");
  diag();
  // TODO: build CN40 command frame
  // TODO: write bytes using write_array()
}

}
