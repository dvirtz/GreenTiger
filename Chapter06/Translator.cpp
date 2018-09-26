#include "Translator.h"
#include "CallingConvention.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {

Translator::Translator(TempMap &tempMap, CallingConvention &callingConvention) :
    m_tempMap(tempMap), m_callingConvention(callingConvention) {
  m_frames.push_back(
    m_callingConvention.createFrame(m_tempMap, "start", BoolList{}));
}

tiger::Translator::Level Translator::outermost() const { return 0; }

tiger::Translator::Level Translator::newLevel(Level /* parent */, Label label,
                                              const BoolList &formals) {
  // add static link
  BoolList withStaticLink = formals;
  withStaticLink.resize(withStaticLink.size() + 1);
  (withStaticLink <<= 1).set(0, true);
  m_frames.push_back(
    m_callingConvention.createFrame(m_tempMap, label, withStaticLink));
  return m_frames.size() - 1;
}

tiger::AccessList Translator::formals(Level level) {
  assert(level < m_frames.size());
  return m_frames[level]->formals();
}

std::pair<tiger::Translator::Level, tiger::VariableAccess>
  Translator::allocateLocal(Level level, bool escapes) {
  assert(level < m_frames.size());
  return {level, m_frames[level]->allocateLocal(escapes)};
}

} // namespace tiger
