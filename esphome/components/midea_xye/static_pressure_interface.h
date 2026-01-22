#pragma once

namespace esphome {
namespace midea {
namespace ac {

class StaticPressureInterface {
 public:
  virtual void set_static_pressure(uint8_t value) = 0;
};

}  // namespace ac
}  // namespace midea
}  // namespace esphome
