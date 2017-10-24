#include "x64FastCallFrameFactory.h"
#include "x64FastCallFrame.h"

namespace tiger {

std::unique_ptr<Frame> x64FastCallFrameFactory::createFrame(TempMap& tempMap, const Label& name, const BoolList & formals)
{
  return std::make_unique<x64FastCall::Frame>(tempMap, name, formals);
}

} // namespace tiger
