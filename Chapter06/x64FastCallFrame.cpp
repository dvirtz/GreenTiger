#include "x64FastCallFrame.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {
namespace x64FastCall {

Frame::Frame(TempMap &tempMap, const Label &name,
             const BoolList &formals)
    : m_tempMap(tempMap), m_name(name) {
  for (size_t i = 0; i < formals.size(); ++i) {
    m_formals.push_back(allocateLocal(formals[i]));
  }
}

Label Frame::name() const { return m_name; }

AccessList Frame::formals() const { return m_formals; }

VariableAccess Frame::allocateLocal(bool escapes) {
  if (escapes || m_allocatedRegs == MAX_REGS) {
    auto res = InFrame{ m_frameOffset };
    m_frameOffset += FRAME_INC;
    return res;
  }

  assert(m_allocatedRegs < MAX_REGS);
  ++m_allocatedRegs;
  return InReg{m_tempMap.newTemp()};
}

} // namespace x64FastCall
} // namespace tiger
