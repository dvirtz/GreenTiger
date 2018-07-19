#pragma once
#include "TempMap.h"
#include <vector>

namespace tiger {
namespace frame {
class CallingConvention;
}

namespace assembly {
class CodeGenerator;
} // namespace assembly

class Machine {
public:
  virtual ~Machine() = default;

  virtual temp::PredefinedRegisters predefinedRegisters() = 0;

  virtual frame::CallingConvention &callingConvention() = 0;
  virtual const frame::CallingConvention &callingConvention() const = 0;

  virtual assembly::CodeGenerator &codeGenerator() = 0;
  virtual const assembly::CodeGenerator &codeGenerator() const = 0;
};
} // namespace tiger