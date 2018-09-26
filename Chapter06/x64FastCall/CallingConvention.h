#pragma once

#include "../CallingConvention.h"

namespace tiger {
class x64FastCallCallingConvention : public CallingConvention {
public:
  // Inherited via CallingConvention
  virtual std::unique_ptr<Frame> createFrame(TempMap &tempMap,
                                             const Label &name,
                                             const BoolList &formals) override;
};
} // namespace tiger
