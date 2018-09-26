#pragma once
#include "TempMap.h"
#include <boost/dynamic_bitset_fwd.hpp>
#include <boost/variant.hpp>
#include <vector>

namespace tiger {

using BoolList = boost::dynamic_bitset<>;

struct InFrame {
  int m_offset;
};

struct InReg {
  Temporary m_reg;
};

using VariableAccess = boost::variant<InFrame, InReg>;
using AccessList     = std::vector<VariableAccess>;

class Frame {
public:
  virtual ~Frame() noexcept                          = default;
  virtual Label name() const                         = 0;
  virtual AccessList formals() const                 = 0;
  virtual VariableAccess allocateLocal(bool escapes) = 0;
};

} // namespace tiger