// Host-side unit tests for the pure conversion logic in XYEAdapter.
//
// These run on the CI host, not the ESP target. xye_adapter.cpp is compiled
// against the minimal stub headers under tests/native/stubs/ that provide just
// the esphome climate enums and logging macros it needs.
//
// Build & run (from the repository root, output kept out of the tree):
//   g++ -std=c++17 -DUSE_ARDUINO \
//       -Itests/native/stubs -Iesphome/components/midea_xye \
//       tests/native/test_get_climate_action.cpp \
//       esphome/components/midea_xye/xye_adapter.cpp -o /tmp/xye_native_tests
//   /tmp/xye_native_tests

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

using esphome::climate::ClimateAction;
using esphome::climate::ClimateMode;
using esphome::midea::xye::FAN_AUTO_FLAG;
using esphome::midea::xye::FanMode;
using esphome::midea::xye::FanSpeedLevel;
using esphome::midea::xye::OperationMode;
using esphome::midea::xye::XYEAdapter;

int g_failures = 0;
int g_checks = 0;

const char *action_name(ClimateAction a) {
  switch (a) {
    case esphome::climate::CLIMATE_ACTION_OFF: return "OFF";
    case esphome::climate::CLIMATE_ACTION_IDLE: return "IDLE";
    case esphome::climate::CLIMATE_ACTION_COOLING: return "COOLING";
    case esphome::climate::CLIMATE_ACTION_HEATING: return "HEATING";
    case esphome::climate::CLIMATE_ACTION_DRYING: return "DRYING";
    case esphome::climate::CLIMATE_ACTION_FAN: return "FAN";
  }
  return "?";
}

void expect_action(const char *desc, ClimateMode mode, FanMode fan, OperationMode op,
                    bool compressor, bool defrost, ClimateAction expected) {
  g_checks++;
  ClimateAction got = XYEAdapter::get_climate_action(mode, fan, op, compressor, defrost);
  if (got != expected) {
    g_failures++;
    std::printf("FAIL: %s -- expected %s, got %s\n", desc, action_name(expected), action_name(got));
  } else {
    std::printf("ok:   %s\n", desc);
  }
}

void expect_level(const char *desc, FanMode fan, FanSpeedLevel expected) {
  g_checks++;
  FanSpeedLevel got = XYEAdapter::get_fan_speed_level(fan);
  if (got != expected) {
    g_failures++;
    std::printf("FAIL: %s -- expected %d, got %d\n", desc, static_cast<int>(expected),
                static_cast<int>(got));
  } else {
    std::printf("ok:   %s\n", desc);
  }
}

}  // namespace

int main() {
  const auto HEAT = ClimateMode::CLIMATE_MODE_HEAT;
  const auto COOL = ClimateMode::CLIMATE_MODE_COOL;
  const auto HEATCOOL = ClimateMode::CLIMATE_MODE_HEAT_COOL;
  const auto OFF = ClimateMode::CLIMATE_MODE_OFF;
  const auto A_HEATING = esphome::climate::CLIMATE_ACTION_HEATING;
  const auto A_COOLING = esphome::climate::CLIMATE_ACTION_COOLING;
  const auto A_IDLE = esphome::climate::CLIMATE_ACTION_IDLE;
  const auto A_FAN = esphome::climate::CLIMATE_ACTION_FAN;
  const auto FAN_ON = FanMode::FAN_HIGH;
  const auto FAN_NONE = FanMode::FAN_OFF;
  const auto OP_NA = OperationMode::OFF;  // operation_mode is ignored unless mode == HEAT_COOL

  std::printf("-- get_climate_action --\n");

  // HEAT: HEATING only while the compressor runs, FAN if it does not, IDLE if the fan is off.
  expect_action("HEAT, fan on, compressor on", HEAT, FAN_ON, OP_NA, true, false, A_HEATING);
  expect_action("HEAT, fan on, compressor off -> FAN", HEAT, FAN_ON, OP_NA, false, false, A_FAN);
  expect_action("HEAT, fan off -> IDLE", HEAT, FAN_NONE, OP_NA, true, false, A_IDLE);
  expect_action("HEAT, compressor on, defrost -> IDLE", HEAT, FAN_ON, OP_NA, true, true, A_IDLE);

  // COOL mirrors HEAT; a defrost cycle must NOT downgrade a cooling action.
  expect_action("COOL, fan on, compressor on", COOL, FAN_ON, OP_NA, true, false, A_COOLING);
  expect_action("COOL, fan on, compressor off -> FAN", COOL, FAN_ON, OP_NA, false, false, A_FAN);
  expect_action("COOL, compressor on, defrost -> still COOLING", COOL, FAN_ON, OP_NA, true, true, A_COOLING);

  // HEAT_COOL resolves the action from the reported sub-mode (operation_mode).
  expect_action("HEAT_COOL/HEAT sub-mode, compressor on", HEATCOOL, FAN_ON, OperationMode::HEAT, true,
                false, A_HEATING);
  expect_action("HEAT_COOL/HEAT sub-mode, compressor off -> FAN", HEATCOOL, FAN_ON, OperationMode::HEAT,
                false, false, A_FAN);
  // Defrost downgrade must reach the HEAT_COOL heating sub-mode, not just plain HEAT.
  expect_action("HEAT_COOL/HEAT sub-mode, defrost -> IDLE", HEATCOOL, FAN_ON, OperationMode::HEAT, true,
                true, A_IDLE);
  expect_action("HEAT_COOL/COOL sub-mode, compressor on", HEATCOOL, FAN_ON, OperationMode::COOL, true,
                false, A_COOLING);
  expect_action("HEAT_COOL/COOL sub-mode, defrost -> still COOLING", HEATCOOL, FAN_ON,
                OperationMode::COOL, true, true, A_COOLING);
  expect_action("HEAT_COOL/FAN sub-mode -> FAN", HEATCOOL, FAN_ON, OperationMode::FAN, true, false, A_FAN);
  // With the indoor fan off, HEAT_COOL reports no active action regardless of sub-mode.
  expect_action("HEAT_COOL/HEAT sub-mode, fan off -> IDLE", HEATCOOL, FAN_NONE, OperationMode::HEAT, true,
                false, A_IDLE);

  // OFF is outside the function's contract but must stay benign.
  expect_action("OFF -> IDLE", OFF, FAN_ON, OP_NA, true, false, A_IDLE);

  std::printf("\n-- get_fan_speed_level --\n");
  expect_level("FAN_OFF -> OFF", FanMode::FAN_OFF, FanSpeedLevel::SPEED_OFF);
  expect_level("FAN_LOW -> LOW", FanMode::FAN_LOW, FanSpeedLevel::SPEED_LOW);
  expect_level("FAN_LOW_ALT -> LOW", FanMode::FAN_LOW_ALT, FanSpeedLevel::SPEED_LOW);
  expect_level("FAN_MEDIUM -> MEDIUM", FanMode::FAN_MEDIUM, FanSpeedLevel::SPEED_MEDIUM);
  expect_level("FAN_HIGH -> HIGH", FanMode::FAN_HIGH, FanSpeedLevel::SPEED_HIGH);
  // The AUTO bit must be masked off; compose the combined byte from the named
  // protocol constants rather than hardcoding its numeric value.
  const auto auto_high = static_cast<FanMode>(FAN_AUTO_FLAG | static_cast<uint8_t>(FanMode::FAN_HIGH));
  expect_level("AUTO|HIGH byte -> HIGH", auto_high, FanSpeedLevel::SPEED_HIGH);

  std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
