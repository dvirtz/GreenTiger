#pragma once
#include "CodeGenerator.h"

namespace tiger {
namespace assembly {
namespace x64 {
class CodeGenerator : public assembly::CodeGenerator {
public:
  CodeGenerator(frame::CallingConvention &callingConvention);

  Instructions translateString(const temp::Label &label,
                               const std::string &string,
                               temp::Map &tempMap) override;

  Instructions translateArgs(const std::vector<ir::Expression> &args, const temp::Map &tempMap) const override;
};
} // namespace x64
} // namespace assembly
} // namespace tiger
