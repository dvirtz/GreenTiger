#pragma once

#include "CallingConvention.h"

namespace tiger {
namespace frame {
namespace m68k {
class CallingConvention final : public frame::CallingConvention {
public:
  std::unique_ptr<frame::Frame>
  createFrame(temp::Map &tempMap, const temp::Label &name,
              const BoolList &formals) override;

  int wordSize() const override;

  temp::Register framePointer() const override;

  temp::Register returnValue() const override;

  temp::Register stackPointer() const override;

  assembly::Registers callDefinedRegisters() const override;

  assembly::Registers calleeSavedRegisters() const override;

  assembly::Registers argumentRegisters() const override;
};
} // namespace m68k
} // namespace frame
} // namespace tiger
