#include "x64CodeGenerator.h"
#include "Tree.h"

namespace tiger {
namespace assembly {
namespace x64 {

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention)
    : assembly::CodeGenerator{callingConvention, {}} {}

Instructions CodeGenerator::translateString(const temp::Label &/* label */,
                                            const std::string &/* string */,
                                            temp::Map &/* tempMap */) {
  return {};
}

Instructions CodeGenerator::translateArgs(const std::vector<ir::Expression> &/* args */) const {
  return {};
}

} // namespace x64
} // namespace assembly
} // namespace tiger
