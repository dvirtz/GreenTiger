#pragma once
#include "Frame.h"

namespace tiger {
namespace assembly {
class Instruction;
using Instructions = std::vector<Instruction>;
} // namespace assembly

namespace frame {
class CallingConvention {
public:
  virtual ~CallingConvention() = default;

  virtual std::unique_ptr<Frame> createFrame(temp::Map &tempMap,
                                             const temp::Label &name,
                                             const BoolList &formals) = 0;
  virtual int wordSize() const                                        = 0;

  virtual temp::Register framePointer() const = 0;

  virtual temp::Register returnValue() const = 0;

  virtual temp::Register stackPointer() const = 0;

  virtual const temp::Registers &callDefinedRegisters() const = 0;

  virtual const temp::Registers &calleeSavedRegisters() const = 0;

  virtual const temp::Registers &argumentRegisters() const = 0;

  ir::Expression accessFrame(const VariableAccess &access,
                             const ir::Expression &framePtr) const;

  ir::Expression externalCall(const temp::Label &name,
                              const std::vector<ir::Expression> &args);

  temp::Registers liveAtExitRegisters() const;

  // calculate register liveness
  assembly::Instructions
    procEntryExit2(const assembly::Instructions &body) const;
};
} // namespace frame
} // namespace tiger
