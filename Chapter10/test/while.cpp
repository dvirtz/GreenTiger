#include "Test.h"

TEST_CASE_METHOD(TestFixture, "while") {
  auto results = checkedCompile(R"(
let
 var b := 2
 var a := 0
in
 while b > 0 do (a:=a+1)
end
)");
  OptReg regs[10];
  OptLabel loopStart, loopDone, trueDest, falseDest, loopNotDone, end;
  OptIndex loopStartIndex, loopDoneIndex, trueDestIndex, falseDestIndex,
    loopNotDoneIndex, endIndex;
  checkProgram(
    results, checkMain(), checkMove(regs[0], 2),       // move 2 to b
    checkMove(regs[1], 0, {regs[0]}),                                        // move 0 to a
    checkLabel(loopStart, loopStartIndex),
    checkMove(regs[2], 1, {regs[0], regs[1]}), // move 1 to r
    checkMove(regs[4], 0, {regs[0], regs[1], regs[2]}),
    checkConditionalJump(ir::RelOp::GT, regs[0], regs[4], trueDest,
                         trueDestIndex, falseDestIndex, {regs[1], regs[2]}), // check condition,
                                                         // jumping to trueDest
                                                         // or falseDest
                                                         // accordingly
    checkLabel(falseDest, falseDestIndex),
    checkMove(regs[2], 0, {regs[0], regs[1]}), // if condition is false, move 0 to r
    checkLabel(trueDest, trueDestIndex),
    checkMove(regs[6], 0, {regs[0], regs[1], regs[2]}),
    checkConditionalJump(
      ir::RelOp::EQ, regs[2], regs[6], loopDone, loopDoneIndex,
      loopNotDoneIndex,
      {regs[0], regs[1]}), // if test equals 0 jumps to loopDone
    checkLabel(loopNotDone, loopNotDoneIndex),
    checkMove(regs[8], 1, {regs[0], regs[1]}),
    checkBinaryOperation(ir::BinOp::PLUS, regs[1], regs[8], regs[9], {regs[0]}),
    checkMove(regs[1], regs[9], {regs[0]}), // a := a + 1
    checkJump(loopStart, loopStartIndex),        // jump back to loop Start
    checkLabel(loopDone, loopDoneIndex),
    checkMove(returnReg(), 0), // return 0
    branchToEnd(end, endIndex), checkFunctionExit());
}