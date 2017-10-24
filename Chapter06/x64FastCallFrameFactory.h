#include "FrameFactory.h"

namespace tiger {
class x64FastCallFrameFactory : public FrameFactory
{
public:
  // Inherited via FrameFactory
  virtual std::unique_ptr<Frame> createFrame(TempMap& tempMap, const Label& name, const BoolList & formals) override;

};
} // namespace tiger
