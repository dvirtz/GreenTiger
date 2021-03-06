#pragma once

#include "Machine.h"
#include "x64CallingConvention.h"
#include "x64CodeGenerator.h"

namespace tiger {
namespace x64 {

class Machine : public tiger::Machine {
public:
  // Inherited via Machine
  virtual frame::CallingConvention &callingConvention() override;
  virtual const frame::CallingConvention &callingConvention() const override;
  virtual assembly::CodeGenerator &codeGenerator() override;
  virtual const assembly::CodeGenerator &codeGenerator() const override;

private:
  frame::x64::CallingConvention m_callingConvention;
  assembly::x64::CodeGenerator m_codeGenerator{m_callingConvention};

  // Inherited via Machine
  virtual temp::PredefinedRegisters predefinedRegisters() const override;
};
} // namespace x64
} // namespace tiger