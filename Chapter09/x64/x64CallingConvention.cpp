#include "x64CallingConvention.h"
#include "x64Frame.h"
#include "x64Registers.h"

namespace tiger
{
namespace frame
{
namespace x64
{

std::unique_ptr<frame::Frame>
CallingConvention::createFrame(temp::Map &tempMap, const temp::Label &name,
                               const BoolList &formals)
{
  return std::make_unique<x64::Frame>(tempMap, name, formals);
}

int CallingConvention::wordSize() const { return Frame::WORD_SIZE; }

temp::Register CallingConvention::framePointer() const
{
  return reg(Registers::RBP);
}

temp::Register CallingConvention::returnValue() const
{
  return reg(Registers::RAX);
}

temp::Register CallingConvention::stackPointer() const
{
  return reg(Registers::RSP);
}

std::vector<temp::Register> CallingConvention::callDefinedRegisters() const
{
  return {reg(Registers::RAX), reg(Registers::RCX), reg(Registers::RDX), reg(Registers::R8),
          reg(Registers::R9), reg(Registers::R10), reg(Registers::R11)};
}

assembly::Registers CallingConvention::calleeSavedRegisters() const
{
  return {reg(Registers::RBX), reg(Registers::RBP), reg(Registers::RDI), reg(Registers::RSI),
          reg(Registers::R12), reg(Registers::R13), reg(Registers::R14), reg(Registers::R15)};
}

assembly::Registers CallingConvention::argumentRegisters() const
{
  return {reg(Registers::RCX), reg(Registers::RDX),
          reg(Registers::R8), reg(Registers::R9)};
}

} // namespace x64
} // namespace frame
} // namespace tiger
