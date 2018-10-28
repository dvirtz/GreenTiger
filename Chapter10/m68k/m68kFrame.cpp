#include "m68kFrame.h"
#include "Assembly.h"
#include "CallingConvention.h"
#include "variantMatch.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {
namespace frame {
namespace m68k {

Frame::Frame(temp::Map &tempMap,
             const frame::CallingConvention &callingConvention,
             const temp::Label &name, const BoolList &formals) :
    frame::Frame{tempMap, callingConvention},
    m_name(name), m_wordSize{callingConvention.wordSize()}, m_frameOffset{
                                                              -m_wordSize} {
  for (size_t i = 0; i < formals.size(); ++i) {
    auto const varAccess = InFrame{m_frameOffset};
    m_formals.emplace_back(varAccess);
    m_frameOffset -= m_wordSize;

    m_parameterMoves.emplace_back(ir::Move{
      ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS,
                                           m_callingConvention.stackPointer(),
                                           static_cast<int>(i * m_wordSize)}},
      m_callingConvention.accessFrame(varAccess,
                                      m_callingConvention.framePointer())});
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

} // namespace m68k
} // namespace frame
} // namespace tiger
