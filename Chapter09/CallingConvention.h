#pragma once
#include "Frame.h"

namespace tiger
{
namespace assembly
{
struct Instruction;
using Instructions = std::vector<Instruction>;
using Registers = std::vector<temp::Register>;
} // namespace assembly

namespace frame
{
class CallingConvention
{
public:
  virtual ~CallingConvention() = default;

  virtual std::unique_ptr<Frame> createFrame(temp::Map &tempMap,
                                             const temp::Label &name,
                                             const BoolList &formals) = 0;
  virtual int wordSize() const = 0;

  virtual temp::Register framePointer() const = 0;

  virtual temp::Register returnValue() const = 0;

  virtual temp::Register stackPointer() const = 0;

  virtual assembly::Registers callDefinedRegisters() const = 0;

  virtual assembly::Registers calleeSavedRegisters() const = 0;

  virtual assembly::Registers argumentRegisters() const = 0;

  ir::Expression accessFrame(const VariableAccess &access,
                             const ir::Expression &framePtr) const;

  ir::Expression
  externalCall(const temp::Label &name,
               const std::vector<ir::Expression> &args);

  // calculate register liveness
  assembly::Instructions procEntryExit2(const assembly::Instructions &body) const;
};
} // namespace frame
} // namespace tiger
