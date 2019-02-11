#include "Test.h"

TEST_CASE_METHOD(TestFixture, "for") {
  auto results = checkedCompile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)");
  OptReg regs[4], staticLink, a, temps[2], functionStartTemps[10];
  OptLabel functionLabel, loopStart, loopDone, functionEnd, end;
  OptIndex functionLabelIndex, loopStartIndex, loopDoneIndex, functionEndIndex,
    endIndex;
  checkProgram(
    results,
    checkFunctionStart(functionLabel, functionLabelIndex, functionStartTemps, escapes{staticLink}, a), // function label
    checkMove(returnReg(), 0),                             // body
    branchToEnd(functionEnd, functionEndIndex), checkFunctionExit(),
    checkMain(), checkMove(regs[0], 0), // move 0 to a
    checkMove(regs[1], 4, {regs[0]}),   // move 4 to limit
    checkConditionalJump(ir::RelOp::GT, regs[0], regs[1], loopDone,
                         loopDoneIndex, loopStartIndex,
                         {regs[1]}), // skip the loop if a , limit
    checkLabel(loopStart, loopStartIndex), checkStaticLink(staticLink, temps, {regs[0], regs[1]}),
    checkCall(checkLabel(functionLabel), {regs[0], regs[1]}, staticLink, regs[0]),
    checkMove(regs[2], 1, {regs[0], regs[1]}),
    checkBinaryOperation(ir::BinOp::PLUS, regs[0], regs[2], regs[3], {regs[1]}),
    checkMove(regs[0], regs[3], {regs[1]}), // a := a + 1
    checkConditionalJump(ir::RelOp::LT, regs[0], regs[1], loopStart,
                         loopStartIndex, loopDoneIndex),
    checkLabel(loopDone, loopDoneIndex), checkMove(returnReg(), 0),
    branchToEnd(end, endIndex), checkFunctionExit());
}