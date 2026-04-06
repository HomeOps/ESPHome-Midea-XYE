#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/defines.h"
#ifdef USE_SWITCH

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"

namespace esphome {
namespace midea {
namespace xye {

// Forward declaration to avoid circular dependency
class ClimateMideaXYE;

class UseFahrenheitSwitch : public switch_::Switch, public Parented<ClimateMideaXYE> {
 public:
  UseFahrenheitSwitch() = default;

 protected:
  void write_state(bool state) override;
};

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_SWITCH
#endif  // USE_ARDUINO
