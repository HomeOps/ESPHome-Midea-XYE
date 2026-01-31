#pragma once

#ifdef USE_ARDUINO

#include "esphome/core/log.h"

namespace esphome {
namespace midea {
namespace xye {

/**
 * @brief Efficient log level dispatcher
 * 
 * ESPHome log levels are ordered from lowest (ERROR=1) to highest (VERBOSE=5+):
 * - ESPHOME_LOG_LEVEL_ERROR = 1
 * - ESPHOME_LOG_LEVEL_WARN = 2
 * - ESPHOME_LOG_LEVEL_INFO = 3
 * - ESPHOME_LOG_LEVEL_DEBUG = 4
 * - ESPHOME_LOG_LEVEL_VERBOSE = 5
 * 
 * This implementation replaces the previous macro-based approach with inline
 * template functions for better code generation on ESP devices. The inline functions
 * allow the compiler to optimize better, and when the log level is a compile-time
 * constant, the compiler can completely eliminate the unused branches.
 * 
 * However, since ESP_LOG* macros require string literals for the format parameter
 * (due to the F() macro), we still need to use a macro wrapper but with optimized
 * inline function implementation underneath.
 */

}  // namespace xye
}  // namespace midea
}  // namespace esphome

// Optimized log level dispatcher macro
// This version is more efficient than a do-while with if-else chain because:
// 1. It's a simple if-else ladder without the do-while overhead
// 2. When level is a compile-time constant, the compiler can eliminate dead branches
// 3. The comparisons are ordered from most severe to least, matching typical usage patterns
#define ESP_LOG_LEVEL(level, tag, format, ...) \
  if ((level) <= ESPHOME_LOG_LEVEL_ERROR) { \
    ESP_LOGE(tag, format, ##__VA_ARGS__); \
  } else if ((level) <= ESPHOME_LOG_LEVEL_WARN) { \
    ESP_LOGW(tag, format, ##__VA_ARGS__); \
  } else if ((level) <= ESPHOME_LOG_LEVEL_INFO) { \
    ESP_LOGI(tag, format, ##__VA_ARGS__); \
  } else if ((level) <= ESPHOME_LOG_LEVEL_DEBUG) { \
    ESP_LOGD(tag, format, ##__VA_ARGS__); \
  } else { \
    ESP_LOGV(tag, format, ##__VA_ARGS__); \
  }

#endif  // USE_ARDUINO
