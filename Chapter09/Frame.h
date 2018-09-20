#pragma once
#include "TempMap.h"
#include "Tree.h"
#include <boost/dynamic_bitset_fwd.hpp>
#include <boost/variant.hpp>
#include <vector>

namespace tiger {

namespace assembly {
struct Instruction;
using Instructions = std::vector<Instruction>;
}

namespace frame {

using BoolList = boost::dynamic_bitset<>;

struct InFrame {
  int m_offset;
};

struct InReg {
  temp::Register m_reg;
};

using VariableAccess = boost::variant<InFrame, InReg>;
using AccessList = std::vector<VariableAccess>;

class Frame {
public:
  Frame(temp::Map& tempMap)
    : m_tempMap(tempMap)
  {}

  virtual ~Frame() noexcept = default;

  virtual temp::Label name() const = 0;
  virtual const AccessList &formals() const = 0;
  virtual VariableAccess allocateLocal(bool escapes) = 0;
  // move input parameters to the function frame
  // store and restore callee saved registers
  virtual ir::Statement procEntryExit1(const ir::Statement& body) const = 0;
  // add prolog and epilog
  virtual assembly::Instructions procEntryExit3(const assembly::Instructions &body) const = 0;
protected:
  temp::Map& m_tempMap;
};

} // namespace frame
} // namespace tiger
