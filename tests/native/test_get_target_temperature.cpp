// Host-side unit tests for XYEAdapter::get_target_temperature().
//
// These run on the CI host, not the ESP target. xye_adapter.cpp is compiled
// against the minimal stub headers under tests/native/stubs/ that provide just
// the esphome climate enums and logging macros it needs.
//
// Build & run (from the repository root, output kept out of the tree):
//   g++ -std=c++17 -DUSE_ARDUINO
//       -Itests/native/stubs -Iesphome/components/midea_xye
//       tests/native/test_get_target_temperature.cpp
//       esphome/components/midea_xye/xye_adapter.cpp -o /tmp/xye_temp_tests
//   /tmp/xye_temp_tests

#include <cmath>
#include <cstdint>
#include <cstdio>

#include "xye_adapter.h"

// xye_adapter.cpp references Temperature::to_celsius via get_temperature(), a
// function these tests do not exercise. A stub definition satisfies the linker.
namespace esphome {
namespace midea {
namespace xye {
float Temperature::to_celsius() const { return 0.0f; }
}  // namespace xye
}  // namespace midea
}  // namespace esphome

namespace {

using esphome::midea::xye::XYEAdapter;

int g_failures = 0;
int g_checks   = 0;

void expect_temp(const char *desc, uint8_t raw, bool use_fahrenheit, float expected_celsius) {
  g_checks++;
  const float got = XYEAdapter::get_target_temperature(raw, use_fahrenheit);
  if (std::fabs(got - expected_celsius) > 0.01f) {
    g_failures++;
    std::printf("FAIL: %s -- expected %.4f°C, got %.4f°C\n", desc, expected_celsius, got);
  } else {
    std::printf("ok:   %s\n", desc);
  }
}

}  // namespace

int main() {
  std::printf("-- get_target_temperature (Fahrenheit branch) --\n");

  // Fahrenheit setpoints: raw = F_value + FAHRENHEIT_TEMP_OFFSET (0x87 = 135).
  // Decode: fahrenheit = raw - 135;  celsius = (fahrenheit - 32) * 5/9.
  expect_temp("0xC8 = 65°F -> 18.33°C", 0xC8, true, (65.0f - 32.0f) * 5.0f / 9.0f);
  expect_temp("0xCB = 68°F -> 20.00°C", 0xCB, true, (68.0f - 32.0f) * 5.0f / 9.0f);
  expect_temp("0xE1 = 90°F -> 32.22°C", 0xE1, true, (90.0f - 32.0f) * 5.0f / 9.0f);

  std::printf("\n-- get_target_temperature (Celsius branch) --\n");

  // Celsius setpoints: SET_TEMP_STATUS_FLAG (bit 6, 0x40) is masked off before display.
  // raw 0x16 (22) — flag clear; 0x56 (86 = 64+22) — flag set; both decode to 22°C.
  expect_temp("0x16 -> 22°C (flag clear)",  0x16, false, 22.0f);
  expect_temp("0x56 -> 22°C (0x40 masked)", 0x56, false, 22.0f);
  // raw 0x1A (26) — flag clear; 0x5A (90 = 64+26) — flag set; both decode to 26°C.
  expect_temp("0x1A -> 26°C (flag clear)",  0x1A, false, 26.0f);
  expect_temp("0x5A -> 26°C (0x40 masked)", 0x5A, false, 26.0f);

  std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
