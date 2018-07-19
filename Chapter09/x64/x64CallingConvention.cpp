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

ir::Expression
CallingConvention::accessFrame(const VariableAccess &access,
                               const ir::Expression &framePtr) const {
  using helpers::match;
  return match(access)(
      [](const frame::InReg &inReg) -> ir::Expression { return inReg.m_reg; },
      [&framePtr](const frame::InFrame &inFrame) -> ir::Expression {
        return ir::MemoryAccess{
            ir::BinaryOperation{ir::BinOp::PLUS, framePtr, inFrame.m_offset}};
      });
}

ir::Expression
CallingConvention::externalCall(const temp::Label &name,
                                const std::vector<ir::Expression> &args) {
  return ir::Call{name, args};
}

std::vector<temp::Register> CallingConvention::callDefinedRegisters() const {
  return {};
}

} // namespace x64
} // namespace frame
} // namespace tiger
