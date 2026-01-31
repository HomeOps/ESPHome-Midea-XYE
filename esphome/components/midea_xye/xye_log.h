#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

/**
 * @brief Helper macro to log at the specified level
 * 
 * ESPHome log levels are ordered from lowest (ERROR=1) to highest (VERBOSE=5+):
 * - ESPHOME_LOG_LEVEL_ERROR = 1
 * - ESPHOME_LOG_LEVEL_WARN = 2
 * - ESPHOME_LOG_LEVEL_INFO = 3
 * - ESPHOME_LOG_LEVEL_DEBUG = 4
 * - ESPHOME_LOG_LEVEL_VERBOSE = 5
 * 
 * This macro dispatches to the appropriate ESP_LOG* macro based on the level.
 */
#define ESP_LOG_LEVEL(level, tag, format, ...) \
  do { \
    if (level <= ESPHOME_LOG_LEVEL_ERROR) { \
      ESP_LOGE(tag, format, ##__VA_ARGS__); \
    } else if (level <= ESPHOME_LOG_LEVEL_WARN) { \
      ESP_LOGW(tag, format, ##__VA_ARGS__); \
    } else if (level <= ESPHOME_LOG_LEVEL_INFO) { \
      ESP_LOGI(tag, format, ##__VA_ARGS__); \
    } else if (level <= ESPHOME_LOG_LEVEL_DEBUG) { \
      ESP_LOGD(tag, format, ##__VA_ARGS__); \
    } else { \
      ESP_LOGV(tag, format, ##__VA_ARGS__); \
    } \
  } while (0)

}  // namespace xye
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
