#pragma once

#include "../CallingConvention.h"

namespace tiger {
namespace frame {
namespace x64FastCall {
class CallingConvention final : public frame::CallingConvention {
public:
  CallingConvention(temp::Map &tempMap);

  virtual std::unique_ptr<frame::Frame>
    createFrame(const temp::Label &name, const BoolList &formals) override;

  virtual int wordSize() const override;

  virtual temp::Register framePointer() const override;

  virtual temp::Register returnValue() const override;

  virtual ir::Expression
    accessFrame(const VariableAccess &access,
                const ir::Expression &framePtr) const override;

  virtual void allocateString(temp::Label label,
                              const std::string &str) override;

  virtual ir::Expression
    externalCall(const std::string &name,
                 const std::vector<ir::Expression> &args) override;
};
} // namespace x64FastCall
} // namespace frame
} // namespace tiger
