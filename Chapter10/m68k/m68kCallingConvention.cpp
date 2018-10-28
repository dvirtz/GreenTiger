#include "m68kCallingConvention.h"
#include "Assembly.h"
#include "m68kFrame.h"
#include "m68kRegisters.h"
#include "variantMatch.h"

namespace tiger {
namespace frame {
namespace m68k {

std::unique_ptr<frame::Frame>
  CallingConvention::createFrame(temp::Map &tempMap, const temp::Label &name,
                                 const BoolList &formals) {
  return std::make_unique<m68k::Frame>(tempMap, *this, name, formals);
}

int CallingConvention::wordSize() const { return 4; }

temp::Register CallingConvention::framePointer() const {
  return reg(Registers::A6);
}

temp::Register CallingConvention::returnValue() const {
  return reg(Registers::D0);
}

temp::Register CallingConvention::stackPointer() const {
  return reg(Registers::A7);
}

const temp::Registers &CallingConvention::callDefinedRegisters() const {
  static const temp::Registers callDefinedRegisters{reg(Registers::A0), reg(Registers::A1), reg(Registers::D0),
          reg(Registers::D1)};
  return callDefinedRegisters;
}

const temp::Registers &CallingConvention::calleeSavedRegisters() const {
  static const temp::Registers calleeSavedRegisters{reg(Registers::A2), reg(Registers::A3), reg(Registers::A4),
          reg(Registers::A5), reg(Registers::A6), reg(Registers::A7),
          reg(Registers::D2), reg(Registers::D3), reg(Registers::D4),
          reg(Registers::D5), reg(Registers::D6), reg(Registers::D7)};
  return calleeSavedRegisters;
}

const temp::Registers &CallingConvention::argumentRegisters() const {
  static const temp::Registers argumentRegisters;
  return argumentRegisters;
}

} // namespace m68k
} // namespace frame
} // namespace tiger
