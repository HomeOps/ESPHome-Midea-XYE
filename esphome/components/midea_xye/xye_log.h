#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/log.h"

// Use ESPHome's built-in runtime log level dispatcher
// The framework provides esp_log_printf_(level, tag, line, format, ...) which
// handles runtime log level selection, eliminating the need for a custom macro.

#endif  // USE_ARDUINO
