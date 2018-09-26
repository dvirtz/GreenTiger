#include "m68kMachine.h"
#include "Tree.h"
#include "m68kRegisters.h"

namespace tiger {
namespace m68k {

frame::CallingConvention &Machine::callingConvention() {
  return m_callingConvention;
}
const frame::CallingConvention &Machine::callingConvention() const {
  return m_callingConvention;
}
assembly::CodeGenerator &Machine::codeGenerator() { return m_codeGenerator; }
const assembly::CodeGenerator &Machine::codeGenerator() const {
  return m_codeGenerator;
}
temp::PredefinedRegisters Machine::predefinedRegisters() {
  using namespace tiger::frame::m68k;
  return {{reg(Registers::D0), "D0"},
          {reg(Registers::D1), "D1"},
          {reg(Registers::D2), "D2"},
          {reg(Registers::D3), "D3"},
          {reg(Registers::D4), "D4"},
          {reg(Registers::D5), "D5"},
          {reg(Registers::D6), "D6"},
          {reg(Registers::D7), "D7"},
          {reg(Registers::A0), "A0"},
          {reg(Registers::A1), "A1"},
          {reg(Registers::A2), "A2"},
          {reg(Registers::A3), "A3"},
          {reg(Registers::A4), "A4"},
          {reg(Registers::A5), "A5"},
          {reg(Registers::A6), "A6"},
          {reg(Registers::A7), "A7"}

  };
}
} // namespace m68k
} // namespace tiger