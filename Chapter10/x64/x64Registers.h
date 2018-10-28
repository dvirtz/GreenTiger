#pragma once
#include "TempMap.h"

namespace tiger {
namespace frame {
namespace x64 {

enum class Registers {
  RAX,
  RDX,
  RCX,
  RBX,
  RSI,
  RDI,
  RBP,
  RSP,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  RIP,
  MM0,
  MM1,
  MM2,
  MM3,
  MM4,
  MM5,
  MM6,
  MM7,
  FP0,
  FP1,
  FP2,
  FP3,
  FP4,
  FP5,
  FP6,
  FP7,
  XMM8,
  XMM9,
  XMM10,
  XMM11,
  XMM12,
  XMM13,
  XMM14,
  XMM15,
  XMM16,
  XMM17,
  XMM18,
  XMM19,
  XMM20,
  XMM21,
  XMM22,
  XMM23,
  XMM24,
  XMM25,
  XMM26,
  XMM27,
  XMM28,
  XMM29,
  XMM30,
  XMM31,
  Size
};

constexpr temp::Register reg(Registers reg) {
  return temp::Register{static_cast<int>(reg)};
}

static_assert(static_cast<int>(Registers::Size) <= temp::MIN_TEMP,
              "MIN_TEMP should be greater than the greatest register");
} // namespace x64
} // namespace frame
} // namespace tiger