#include "Frame.h"

namespace tiger {
class FrameFactory {
public:
  virtual std::unique_ptr<Frame> createFrame(TempMap& tempMap, const Label& name, const BoolList &formals) = 0;
};
} // namespace tiger
