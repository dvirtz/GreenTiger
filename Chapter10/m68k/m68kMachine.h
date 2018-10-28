#pragma once

#include "Machine.h"
#include "m68kCallingConvention.h"
#include "m68kCodeGenerator.h"

namespace tiger {
namespace m68k {

class Machine : public tiger::Machine {
public:
  // Inherited via Machine
  virtual frame::CallingConvention &callingConvention() override;
  virtual const frame::CallingConvention &callingConvention() const override;

  virtual assembly::CodeGenerator &codeGenerator() override;
  virtual const assembly::CodeGenerator &codeGenerator() const override;

private:
  frame::m68k::CallingConvention m_callingConvention;
  assembly::m68k::CodeGenerator m_codeGenerator{m_callingConvention};

  // Inherited via Machine
  virtual temp::PredefinedRegisters predefinedRegisters() const override;
};
} // namespace m68k
} // namespace tiger