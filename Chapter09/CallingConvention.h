#pragma once
#include "Frame.h"

namespace tiger {
namespace assembly {
class MachineInstruction;
}

namespace frame {
class CallingConvention {
public:
  virtual ~CallingConvention() = default;

  virtual std::unique_ptr<Frame> createFrame(temp::Map &tempMap,
                                             const temp::Label &name,
                                             const BoolList &formals) = 0;
  virtual int wordSize() const = 0;

  virtual temp::Register framePointer() const = 0;

  virtual temp::Register returnValue() const = 0;

  virtual ir::Expression accessFrame(const VariableAccess &access,
                                     const ir::Expression &framePtr) const = 0;

  virtual ir::Expression
  externalCall(const temp::Label &name,
               const std::vector<ir::Expression> &args) = 0;

  virtual std::vector<temp::Register> callDefinedRegisters() const = 0;
};
} // namespace frame
} // namespace tiger
