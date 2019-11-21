#include "CallingConvention.h"
#include "Assembly.h"
#include <range/v3/view.hpp>

namespace tiger {
namespace frame {

ir::Expression
  CallingConvention::accessFrame(const VariableAccess &access,
                                 const ir::Expression &framePtr) const {
  using helpers::match;
  return match(access)(
    [](const frame::InReg &inReg) -> ir::Expression { return inReg.m_reg; },
    [&framePtr](const frame::InFrame &inFrame) -> ir::Expression {
      return ir::MemoryAccess{
        ir::BinaryOperation{ir::BinOp::PLUS, framePtr, inFrame.m_offset}};
    });
}

ir::Expression
  CallingConvention::externalCall(const temp::Label &name,
                                  const std::vector<ir::Expression> &args) {
  return ir::Call{name, args};
}

assembly::Instructions
  CallingConvention::procEntryExit2(const assembly::Instructions &body) const {
  // appends a “sink” instruction to the function body to tell the
  // register allocator that certain registers are live at procedure exit
  namespace rv     = ranges::views;
  auto calleeSaved = calleeSavedRegisters();
  auto liveAtExit  = rv::concat(calleeSaved, rv::single(returnValue()),
                               rv::single(stackPointer()));
#ifdef _MSC_VER
  auto res = body;
  res.push_back(assembly::Operation{{}, {}, liveAtExit});
  return res;
#else
  return ranges::views::concat(
    body, rv::single(assembly::Operation{{}, {}, liveAtExit}));
#endif
}

} // namespace frame
} // namespace tiger