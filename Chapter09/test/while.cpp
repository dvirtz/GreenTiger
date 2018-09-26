#include "Test.h"

TEST_CASE("while") {
  auto program = checkedCompile(R"(
let
 var b := 2
 var a := 0
in
 while b > 0 do (a:=a+1)
end
)");
  OptReg regs[10];
  OptLabel loopStart, loopDone, trueDest, falseDest, loopNotDone, end;
  checkProgram(
    program,
    checkMove( // move 2 to b
      checkReg(regs[0]), checkImm(2))
      > checkMove( // move 0 to a
          checkReg(regs[1]), checkImm(0))
      > checkLabel(loopStart) > ':' > checkMove(checkReg(regs[2]),
                                                checkImm(1))
      > // move 1 to r
      checkMove(checkReg(regs[4]), checkImm(0))
      > checkConditionalJump(ir::RelOp::GT, regs[0], regs[4], trueDest)
      > // check condition,
        // jumping to trueDest
        // or falseDest
        // accordingly
      checkLabel(falseDest) > ':' > checkMove(checkReg(regs[2]),
                                              checkImm(0))
      > // if condition is false, move 0 to r
      checkLabel(trueDest) > ':' > checkMove(checkReg(regs[6]), checkImm(0))
      > checkConditionalJump( // test
          ir::RelOp::EQ, regs[2], regs[6], loopDone)
      > // if test equals 0 jumps to loopDone
      checkLabel(loopNotDone) > ':' > checkMove(checkReg(regs[8]), checkImm(1))
      > checkBinaryOperation(ir::BinOp::PLUS, checkReg(regs[1]),
                             checkReg(regs[8]), regs[9])
      > checkMove( // a := a + 1
          checkReg(regs[1]), checkReg(regs[9]))
      > checkJump( // jump back to loop Start
          loopStart)
      > checkLabel(loopDone) > ':' > checkMove( // return 0
                                       returnReg(), checkImm(0)));
}