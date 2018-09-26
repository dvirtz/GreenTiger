#pragma once
#include "TempMap.h"
#include "Tree.h"
#include <boost/dynamic_bitset_fwd.hpp>
#include <boost/variant.hpp>
#include <vector>

namespace tiger {

namespace frame {

using BoolList = boost::dynamic_bitset<>;

struct InFrame {
  int m_offset;
};

struct InReg {
  temp::Register m_reg;
};

using VariableAccess = boost::variant<InFrame, InReg>;
using AccessList     = std::vector<VariableAccess>;

class Frame {
public:
  Frame(temp::Map &tempMap) : m_tempMap(tempMap) {}

  virtual ~Frame() noexcept = default;

  virtual temp::Label name() const                                = 0;
  virtual const AccessList &formals() const                       = 0;
  virtual VariableAccess allocateLocal(bool escapes)              = 0;
  virtual ir::Statement procEntryExit1(const ir::Statement &body) = 0;

protected:
  temp::Map &m_tempMap;
};

} // namespace frame
} // namespace tiger
