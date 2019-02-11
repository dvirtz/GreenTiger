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

  Frame(temp::Map &tempMap, const frame::CallingConvention &callingConvention,
        const temp::Label &name, const BoolList &formals);

  // Inherited via Frame
  virtual temp::Label name() const override;

  virtual const AccessList &formals() const override;

  virtual VariableAccess allocateLocal(bool escapes) override;

private:
  temp::Label m_name;
  AccessList m_formals;
  int m_wordSize;
  int m_frameOffset;
};
} // namespace x64
} // namespace frame
} // namespace tiger
