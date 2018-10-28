#pragma once

#include "Frame.h"
#include "m68kRegisters.h"
#include <array>

namespace tiger {
namespace frame {
namespace m68k {
// implements
// https://docs.microsoft.com/en-gb/cpp/build/overview-of-x64-calling-conventions
class Frame final : public frame::Frame {
public:

  Frame(temp::Map &tempMap, const frame::CallingConvention &callingConvention,
        const temp::Label &name, const BoolList &formals);

  // Inherited via Frame
  temp::Label name() const override;

  const AccessList &formals() const override;

  VariableAccess allocateLocal(bool escapes) override;

private:
  temp::Label m_name;
  AccessList m_formals;
  int m_wordSize;
  int m_frameOffset;
};
} // namespace m68k
} // namespace frame
} // namespace tiger
