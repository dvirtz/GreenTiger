#include "m68kCallingConvention.h"
#include "m68kFrame.h"
#include "m68kRegisters.h"
#include "variantMatch.h"

namespace tiger {
namespace frame {
namespace m68k {

std::unique_ptr<frame::Frame>
CallingConvention::createFrame(temp::Map &tempMap, const temp::Label &name,
                               const BoolList &formals) {
  return std::make_unique<m68k::Frame>(tempMap, name, formals);
}

int CallingConvention::wordSize() const { return Frame::WORD_SIZE; }

temp::Register CallingConvention::framePointer() const {
  return reg(Registers::A6);
}

temp::Register CallingConvention::returnValue() const {
  return reg(Registers::D0);
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
  return {reg(Registers::A0), reg(Registers::A1), reg(Registers::D0),
          reg(Registers::D1)};
}
} // namespace m68k
} // namespace frame
} // namespace tiger
