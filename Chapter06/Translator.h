#pragma once
#include "Frame.h"

namespace tiger {

class CallingConvention;

class Translator {
public:
  using Level = size_t;

  Translator(TempMap &tempMap, CallingConvention &callingConvention);

  Level outermost() const;
  Level newLevel(Level parent, Label label, const BoolList &formals);
  AccessList formals(Level level);
  std::pair<Level, VariableAccess> allocateLocal(Level level, bool escapes);

private:
  TempMap &m_tempMap;
  CallingConvention &m_callingConvention;
  std::vector<std::unique_ptr<Frame>> m_frames;
};
} // namespace tiger
