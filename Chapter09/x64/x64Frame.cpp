#include "x64Frame.h"
#include "variantMatch.h"
#include "Assembly.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {
namespace frame {
namespace x64 {

std::array<Registers, Frame::MAX_REGS> Frame::m_regParams = {
    Registers::RCX, Registers::RDX, Registers::R8, Registers::R9};

Frame::Frame(temp::Map &tempMap, const temp::Label &name,
             const BoolList &formals)
    : frame::Frame(tempMap), m_name(name) {
  for (size_t i = 0; i < formals.size(); ++i) {
    auto varAccess = [this](bool escapes) -> VariableAccess {
      if (escapes || m_allocatedRegs == MAX_REGS) {
        auto res = InFrame{m_frameOffset};
        m_frameOffset -= WORD_SIZE;
        return res;
      }

      assert(m_allocatedRegs < MAX_REGS);
      return InReg{reg(m_regParams[m_allocatedRegs++])};
    }(formals[i]);
    m_formals.push_back(varAccess);
  }
}

temp::Label Frame::name() const { return m_name; }

const AccessList &Frame::formals() const { return m_formals; }

VariableAccess Frame::allocateLocal(bool escapes) {
  if (escapes) {
    auto res = InFrame{m_frameOffset};
    m_frameOffset -= WORD_SIZE;
    return res;
  }

  return InReg{m_tempMap.newTemp()};
}

ir::Statement Frame::procEntryExit1(const ir::Statement &body) const { return body; }

assembly::Instructions Frame::procEntryExit3(const assembly::Instructions &body) const {
  return body;
}

} // namespace x64
} // namespace frame
} // namespace tiger
