#include "CallingConvention.h"
#include "Frame.h"

namespace tiger {

std::unique_ptr<Frame>
  x64FastCallCallingConvention::createFrame(TempMap &tempMap, const Label &name,
                                            const BoolList &formals) {
  return std::make_unique<x64FastCall::Frame>(tempMap, name, formals);
}

} // namespace tiger
