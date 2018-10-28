#include "Test.h"

TEST_CASE_METHOD(TestFixture, "break") {
  SECTION("for") {
    auto results = checkedCompile("for a := 0 to 2 do break");
    OptReg regs[4];
    OptLabel functionLabel, loopStart, loopDone, afterBreak, postLoop, end;
    OptIndex functionLabelIndex, loopStartIndex, loopDoneIndex, afterBreakIndex,
      postLoopIndex, endIndex;
    checkProgram(
      results, checkMain(), checkMove(regs[0], 0), // move 0 to a
      checkMove(regs[1], 2, {regs[0]}),                                  // move 2 to limit
      checkConditionalJump(ir::RelOp::GT, regs[0], regs[1], loopDone,
                           loopDoneIndex, loopStartIndex,
                           {regs[0], regs[1]}), // skip the loop if a , limit
      checkLabel(loopStart, loopStartIndex),
      checkLabel(loopDone, loopDoneIndex), checkMove(returnReg(), 0),
      checkJump(end, endIndex), checkLabel(afterBreak, afterBreakIndex),
      checkMove(regs[2], 1, {regs[0], regs[1]}),
      checkBinaryOperation(ir::BinOp::PLUS, regs[0], regs[2], regs[3], {regs[1]}),
      checkMove(regs[0], regs[3], {regs[1]}), // a := a + 1
      checkConditionalJump(ir::RelOp::LT, regs[0], regs[1], loopStart,
                           loopStartIndex, postLoopIndex),
      checkLabel(postLoop, postLoopIndex), checkJump(loopDone, loopDoneIndex),
      checkLabel(end, endIndex), checkFunctionExit());
  }

  SECTION("while") {
    auto results = checkedCompile("while 1 do break");
    OptReg regs[2];
    OptLabel loopStart, loopDone, loopNotDone, afterBreak, end;
    OptIndex loopStartIndex, loopDoneIndex, loopNotDoneIndex, afterBreakIndex,
      endIndex;
    checkProgram(
      results, checkMain(), checkLabel(loopStart, loopStartIndex),
                 checkMove(regs[0], 1), checkMove(regs[1], 0, {regs[0]}),
      checkConditionalJump( // test
        ir::RelOp::EQ, regs[0], regs[1], loopDone, loopDoneIndex,
        loopNotDoneIndex), // if test equals 0 jumps to loopDone
      checkLabel(loopNotDone, loopNotDoneIndex),
      checkLabel(loopDone, loopDoneIndex), checkMove(returnReg(), 0),
      checkJump(end, endIndex), checkLabel(afterBreak, afterBreakIndex),
      checkJump( // jump back to loop Start
        loopStart, loopStartIndex),
      checkLabel(end, endIndex), checkFunctionExit());
  }
}