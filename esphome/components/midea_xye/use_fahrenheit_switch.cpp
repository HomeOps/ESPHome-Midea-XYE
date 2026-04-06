#ifdef USE_ARDUINO

#include "esphome/core/defines.h"
#ifdef USE_SWITCH

#include "use_fahrenheit_switch.h"
#include "climate_midea_xye.h"

namespace esphome {
namespace midea {
namespace xye {

void UseFahrenheitSwitch::write_state(bool state) {
  this->parent_->set_use_fahrenheit(state);
  this->publish_state(state);
}

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_SWITCH
#endif  // USE_ARDUINO
