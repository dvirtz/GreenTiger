#pragma once
#include "CodeGenerator.h"

namespace tiger {
namespace assembly {
namespace m68k {
class CodeGenerator : public assembly::CodeGenerator {
public:
  CodeGenerator(frame::CallingConvention &callingConvention);

  Instructions translateString(const temp::Label &label,
                               const std::string &string,
                               temp::Map &tempMap) override;

private:
  Instructions translateArgs(const std::vector<ir::Expression> &args, const temp::Map &tempMap) const override;
};
} // namespace m68k
} // namespace assembly
} // namespace tiger
