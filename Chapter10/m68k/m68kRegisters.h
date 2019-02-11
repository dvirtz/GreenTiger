#pragma once
#include "TempMap.h"

namespace tiger {
namespace frame {
namespace m68k {

enum class Registers {
  D0,
  D1,
  D2,
  D3,
  D4,
  D5,
  D6,
  D7,
  A0,
  A1,
  A2,
  A3,
  A4,
  A5,
  A6,
  A7,
  Size
};

constexpr temp::Register reg(Registers reg) {
  return temp::Register{static_cast<int>(reg)};
}

static_assert(static_cast<int>(Registers::Size) <= temp::MIN_TEMP,
              "MIN_TEMP should be greater than the greatest register");
} // namespace m68k
} // namespace frame
} // namespace tiger