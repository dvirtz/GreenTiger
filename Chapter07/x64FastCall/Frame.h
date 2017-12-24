#pragma once

#include "../Frame.h"
#include "Registers.h"
#include <array>

namespace tiger {
namespace frame {
namespace x64FastCall {
// implements
// https://docs.microsoft.com/en-gb/cpp/build/overview-of-x64-calling-conventions
class Frame : public frame::Frame {
public:
  static const int WORD_SIZE = 4;

  Frame(temp::Map &tempMap, const temp::Label &name, const BoolList &formals);
  
  // Inherited via Frame
  virtual temp::Label name() const override;

  virtual AccessList formals() const override;

  virtual VariableAccess allocateLocal(bool escapes) override;

  virtual ir::Statement procEntryExit1(const ir::Statement& body) override;

private:
  temp::Label m_name;
  AccessList m_formals;
  int m_frameOffset = -WORD_SIZE;
  // The __fastcall convention uses registers for the first four arguments
  static const size_t MAX_REGS = 4;
  static std::array<Registers, MAX_REGS> m_regParams;
  size_t m_allocatedRegs = 0;
};
} // namespace x64FastCall
} // namespace frame
} // namespace tiger
