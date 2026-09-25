#include "Quadrature.h"

#include <esp_attr.h>

namespace {

enum : uint8_t {
  START = 0,
  CW_FINAL,
  CW_BEGIN,
  CW_NEXT,
  CCW_BEGIN,
  CCW_FINAL,
  CCW_NEXT,
};
constexpr uint8_t EMIT_CW = 0x10;
constexpr uint8_t EMIT_CCW = 0x20;

// Rows are states, columns the new pin reading 0-3. The table lives in DRAM
// so the ISR can use it while flash is busy (an NVS write, for instance).
DRAM_ATTR const uint8_t kTable[7][4] = {
    // 00          01         10          11
    {START, CW_BEGIN, CCW_BEGIN, START},                // START
    {CW_NEXT, START, CW_FINAL, START | EMIT_CW},        // CW_FINAL
    {CW_NEXT, CW_BEGIN, START, START},                  // CW_BEGIN
    {CW_NEXT, CW_BEGIN, CW_FINAL, START},               // CW_NEXT
    {CCW_NEXT, START, CCW_BEGIN, START},                // CCW_BEGIN
    {CCW_NEXT, CCW_FINAL, START, START | EMIT_CCW},     // CCW_FINAL
    {CCW_NEXT, CCW_FINAL, CCW_BEGIN, START},            // CCW_NEXT
};

}  // namespace

IRAM_ATTR int8_t QuadratureDecoder::step(uint8_t pins) {
  uint8_t next = kTable[_state & 0x0f][pins & 0x03];
  _state = next & 0x0f;
  if (next & EMIT_CW) return 1;
  if (next & EMIT_CCW) return -1;
  return 0;
}
