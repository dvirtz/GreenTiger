#pragma once

#include "Frame.h"
#include "x64Registers.h"
#include <array>

namespace tiger {
namespace frame {
namespace x64 {
// implements
// https://docs.microsoft.com/en-gb/cpp/build/overview-of-x64-calling-conventions
class Frame final : public frame::Frame {
public:
  static const int WORD_SIZE = 8;

  Frame(temp::Map &tempMap, const temp::Label &name, const BoolList &formals);

  // Inherited via Frame
  virtual temp::Label name() const override;

  virtual const AccessList &formals() const override;

  virtual VariableAccess allocateLocal(bool escapes) override;

  virtual ir::Statement
    procEntryExit1(const ir::Statement &body) const override;

  assembly::Instructions
    procEntryExit3(const assembly::Instructions &body) const override;

private:
  temp::Label m_name;
  AccessList m_formals;
  int m_frameOffset = -WORD_SIZE;
  // The __fastcall convention uses registers for the first four arguments
  static const size_t MAX_REGS = 4;
  size_t m_allocatedRegs       = 0;
};
} // namespace x64
} // namespace frame
} // namespace tiger
