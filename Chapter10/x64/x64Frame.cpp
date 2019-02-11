#include "x64Frame.h"
#include "Assembly.h"
#include "CallingConvention.h"
#include "variantMatch.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {
namespace frame {
namespace x64 {

Frame::Frame(temp::Map &tempMap,
             const frame::CallingConvention &callingConvention,
             const temp::Label &name, const BoolList &formals) :
    frame::Frame{tempMap, callingConvention},
    m_name{name}, m_wordSize{callingConvention.wordSize()}, m_frameOffset{
                                                              -m_wordSize} {
  for (size_t i = 0; i < formals.size(); ++i) {
    const auto &argumentRegisters = callingConvention.argumentRegisters();
    auto const varAccess          = [this, &argumentRegisters,
                            i](bool escapes) -> VariableAccess {
      if (escapes || i >= argumentRegisters.size()) {
        auto res = InFrame{m_frameOffset};
        m_frameOffset -= m_wordSize;
        return res;
      }

      auto temp = m_tempMap.newTemp();
      return InReg{temp};
    }(formals[i]);
    m_formals.emplace_back(varAccess);

    auto const argumentSource = [&]() -> ir::Expression {
      if (i < argumentRegisters.size()) {
        return argumentRegisters[i];
      }
      return ir::MemoryAccess{
        ir::BinaryOperation{ir::BinOp::PLUS, m_callingConvention.stackPointer(),
                            static_cast<int>(i * m_wordSize)}};
    }();
    m_parameterMoves.emplace_back(ir::Move{
      argumentSource, m_callingConvention.accessFrame(
                        varAccess, m_callingConvention.framePointer())});
  }
}

temp::Label Frame::name() const { return m_name; }

const AccessList &Frame::formals() const { return m_formals; }

VariableAccess Frame::allocateLocal(bool escapes) {
  if (escapes) {
    auto res = InFrame{m_frameOffset};
    m_frameOffset -= m_wordSize;
    return res;
  }

  return InReg{m_tempMap.newTemp()};
}

} // namespace x64
} // namespace frame
} // namespace tiger
