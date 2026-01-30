#pragma once

#include "esphome/components/number/number.h"
#include "static_pressure_interface.h"

namespace esphome {
namespace midea {
namespace xye {

class StaticPressureNumber : public number::Number {
 public:
  StaticPressureNumber() = default;

  void set_parent(StaticPressureInterface *parent) { this->parent_ = parent; }

  void control(float value) override {
    this->publish_state(value);
    if (parent_) {
      parent_->set_static_pressure(static_cast<uint8_t>(value));
    }
  }

 protected:
  StaticPressureInterface *parent_{nullptr};
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome
