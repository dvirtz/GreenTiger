#include "Test.h"

TEST_CASE("for") {
  auto program = checkedCompile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)");
  OptReg regs[4], staticLink, temps[2];
  OptLabel functionLabel, loopStart, loopDone, functionEnd, end;
  checkProgram(
    program, checkLabel(functionLabel), ':', // function label
    checkMove(                               // body
      returnReg(), checkImm(0)),
    branchToEnd(functionEnd), checkMain(),
    checkMove( // move 0 to a
      checkReg(regs[0]), checkImm(0)),
    checkMove( // move 4 to limit
      checkReg(regs[1]), checkImm(4)),
    checkConditionalJump( // skip the loop if a , limit
      ir::RelOp::GT, regs[0], regs[1], loopDone),
    checkLabel(loopStart), ':',
    checkStaticLink(staticLink, temps),
    checkCall(checkLabel(functionLabel), checkArg(0, checkReg(staticLink))
                                           > checkArg(1, checkReg(regs[0]))),
    checkMove(checkReg(regs[2]), checkImm(1)),
    checkBinaryOperation(ir::BinOp::PLUS, checkReg(regs[0]), checkReg(regs[2]),
                         regs[3]),
    checkMove( // a := a + 1
      checkReg(regs[0]), checkReg(regs[3])),
    checkConditionalJump(ir::RelOp::LT, regs[0], regs[1], loopStart),
    checkLabel(loopDone), ':', checkMove(returnReg(), checkImm(0)),
    branchToEnd(end));
}