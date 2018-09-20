#include "x64CallingConvention.h"
#include "x64Frame.h"
#include "x64Registers.h"

namespace tiger {
namespace frame {
namespace x64 {

std::unique_ptr<frame::Frame>
CallingConvention::createFrame(temp::Map &tempMap, const temp::Label &name,
                               const BoolList &formals) {
  return std::make_unique<x64::Frame>(tempMap, name, formals);
}

int CallingConvention::wordSize() const { return Frame::WORD_SIZE; }

temp::Register CallingConvention::framePointer() const {
  return reg(Registers::RBP);
}

temp::Register CallingConvention::returnValue() const {
  return reg(Registers::RAX);
}

temp::Register CallingConvention::stackPointer() const
{
  return reg(Registers::RSP);
}

std::vector<temp::Register> CallingConvention::callDefinedRegisters() const {
  return {};
}

assembly::Registers CallingConvention::calleeSavedRegisters() const
{
  return {};
}

} // namespace x64
} // namespace frame
} // namespace tiger
