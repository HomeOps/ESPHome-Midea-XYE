#pragma once
// Minimal stub of esphome/core/log.h for host-side unit tests.
// Only the symbols referenced by xye.h are provided. The real logging
// implementation is part of the ESPHome firmware build and is not needed
// when exercising pure adapter logic on the CI host.

#define ESPHOME_LOG_LEVEL_DEBUG 4
#define ESPHOME_LOG_FORMAT(fmt) fmt

namespace esphome {

// No-op stand-in; xye.h's debug-print helpers reference it but the unit
// tests never call them, so this is here purely to satisfy compilation.
inline void esp_log_printf_(int /*level*/, const char * /*tag*/, int /*line*/,
                            const char * /*format*/, ...) {}

}  // namespace esphome
