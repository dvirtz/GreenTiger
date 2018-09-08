#include "CallingConvention.h"
#include "Frame.h"
#include "Registers.h"

namespace tiger {
namespace frame {
namespace x64FastCall {
CallingConvention::CallingConvention(temp::Map &tempMap)
    : frame::CallingConvention(tempMap) {}

std::unique_ptr<frame::Frame>
CallingConvention::createFrame(const temp::Label &name,
                               const BoolList &formals) {
  return std::make_unique<x64FastCall::Frame>(m_tempMap, name, formals);
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

void CallingConvention::allocateString(temp::Label /* label */,
                                       const std::string &/* str */) {
  // TODO
}

ir::Expression
CallingConvention::externalCall(const std::string &name,
                                const std::vector<ir::Expression> &args) {
  return ir::Call{m_tempMap.namedLabel(name), args};
}
} // namespace x64FastCall
} // namespace frame
} // namespace tiger
