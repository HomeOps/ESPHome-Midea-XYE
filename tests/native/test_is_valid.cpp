// Host-side unit tests for ReceiveData::is_valid().
//
// Regression coverage for the reply-direction byte. A real Midea indoor unit on
// a live 5-indoor-unit Mini VRF system (outdoor MD17I-004C / GDV-V160W, indoors
// MD17I-017HW) was captured answering C0 QUERY frames with the direction byte
// set to 0x80 (bit 7 = "this is a reply") rather than 0x00. is_valid() must
// accept that frame; the exact bytes below are copied from that capture.
//
// Build & run (from the repository root, output kept out of the tree):
//   g++ -std=c++17 -DUSE_ARDUINO
//       -Itests/native/stubs -Iesphome/components/midea_xye
//       tests/native/test_is_valid.cpp
//       esphome/components/midea_xye/xye_recv.cpp
//       esphome/components/midea_xye/xye.cpp
//       esphome/components/midea_xye/xye_adapter.cpp -o /tmp/xye_is_valid_tests
//   /tmp/xye_is_valid_tests

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "xye_recv.h"

namespace {

using esphome::midea::xye::ReceiveData;
using esphome::midea::xye::RX_MESSAGE_LENGTH;

int g_failures = 0;
int g_checks = 0;

// A genuine C0 QUERY response captured from a real MD17I-017HW indoor unit
// (address 0x01) on a 5-unit VRF system. Note byte[2] == 0x80: the unit sets
// the reply bit on the direction field. CRC (byte[30]) == 0xE6, prologue 0x55.
const uint8_t REAL_UNIT_C0_REPLY[RX_MESSAGE_LENGTH] = {
    0xAA, 0xC0, 0x80, 0x00, 0x01, 0x00, 0xE0, 0x14, 0x00, 0x00, 0x18,
    0x5C, 0x34, 0x36, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xFF, 0xFF, 0xE6, 0x55};

void expect(const char *desc, bool got, bool expected) {
  g_checks++;
  if (got != expected) {
    g_failures++;
    std::printf("FAIL: %s -- expected %s, got %s\n", desc, expected ? "valid" : "invalid",
                got ? "valid" : "invalid");
  } else {
    std::printf("ok:   %s\n", desc);
  }
}

ReceiveData from_bytes(const uint8_t (&bytes)[RX_MESSAGE_LENGTH]) {
  ReceiveData rx;
  std::memcpy(rx.raw, bytes, RX_MESSAGE_LENGTH);
  return rx;
}

}  // namespace

int main() {
  std::printf("-- ReceiveData::is_valid --\n");

  // The real-world frame with the reply bit set must validate.
  expect("real MD17I-017HW C0 reply (direction 0x80)", from_bytes(REAL_UNIT_C0_REPLY).is_valid(), true);

  // The same frame with a plain 0x00 direction (other units' encoding) still
  // validates. Clearing bit 7 changes the CRC, so recompute it.
  {
    uint8_t bytes[RX_MESSAGE_LENGTH];
    std::memcpy(bytes, REAL_UNIT_C0_REPLY, RX_MESSAGE_LENGTH);
    bytes[2] = 0x00;                 // direction without the reply bit
    bytes[30] = static_cast<uint8_t>(bytes[30] + 0x80);  // CRC = 0xFF - sum; dropping 0x80 raises it by 0x80
    expect("C0 reply with plain 0x00 direction", from_bytes(bytes).is_valid(), true);
  }

  // A corrupted CRC must be rejected.
  {
    uint8_t bytes[RX_MESSAGE_LENGTH];
    std::memcpy(bytes, REAL_UNIT_C0_REPLY, RX_MESSAGE_LENGTH);
    bytes[30] ^= 0xFF;
    expect("corrupted CRC is rejected", from_bytes(bytes).is_valid(), false);
  }

  // A bad preamble must be rejected.
  {
    uint8_t bytes[RX_MESSAGE_LENGTH];
    std::memcpy(bytes, REAL_UNIT_C0_REPLY, RX_MESSAGE_LENGTH);
    bytes[0] = 0x00;
    expect("bad preamble is rejected", from_bytes(bytes).is_valid(), false);
  }

  // A bad prologue must be rejected.
  {
    uint8_t bytes[RX_MESSAGE_LENGTH];
    std::memcpy(bytes, REAL_UNIT_C0_REPLY, RX_MESSAGE_LENGTH);
    bytes[31] = 0x00;
    expect("bad prologue is rejected", from_bytes(bytes).is_valid(), false);
  }

  std::printf("\n%d checks, %d failure(s)\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}
