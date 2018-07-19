#pragma once

#include "CallingConvention.h"

namespace tiger {
namespace frame {
namespace m68k {
class CallingConvention final : public frame::CallingConvention {
public:
  virtual std::unique_ptr<frame::Frame>
  createFrame(temp::Map &tempMap, const temp::Label &name,
              const BoolList &formals) override;

  virtual int wordSize() const override;

  virtual temp::Register framePointer() const override;

  virtual temp::Register returnValue() const override;

  virtual ir::Expression
  accessFrame(const VariableAccess &access,
              const ir::Expression &framePtr) const override;

  virtual ir::Expression
  externalCall(const temp::Label &name,
               const std::vector<ir::Expression> &args) override;

  std::vector<temp::Register> callDefinedRegisters() const override;
};
} // namespace m68k
} // namespace frame
} // namespace tiger
