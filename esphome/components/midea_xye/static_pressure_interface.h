#pragma once

namespace esphome {
namespace midea {
namespace xye {

class StaticPressureInterface {
 public:
  virtual void set_static_pressure(uint8_t value) = 0;
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome
