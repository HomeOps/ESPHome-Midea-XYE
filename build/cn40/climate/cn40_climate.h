#pragma once

#include "esphome.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

namespace cn40 {

class CN40Climate : public esphome::climate::Climate,
                    public esphome::Component,
                    public esphome::uart::UARTDevice {
 public:
  CN40Climate(esphome::uart::UARTComponent *parent);

  void setup() override;
  void loop() override;
  esphome::climate::ClimateTraits traits() override;
  void control(const esphome::climate::ClimateCall &call) override;

 protected:
  std::vector<uint8_t> buffer_;
  void parse_frame_();
  void send_command_();
 private:
  void diag();
};

}

// #include "cn40_climate.cpp"

