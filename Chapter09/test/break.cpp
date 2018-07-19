#include "Test.h"

TEST_CASE("break")
{
    SECTION("for")
    {
        auto program = checkedCompile("for a := 0 to 2 do break");
        OptReg regs[4];
        OptLabel functionLabel, loopStart, loopDone, afterBreak, postLoop, end;
        checkProgram(program,
                     checkMove( // move 0 to a
                         checkReg(regs[0]), checkImm(0)) >
                         checkMove( // move 4 to limit
                             checkReg(regs[1]), checkImm(2)) >
                         checkConditionalJump( // skip the loop if a > limit
                             ir::RelOp::GT, regs[0], regs[1], loopDone) > checkLabel(loopStart) > ':' >
                         checkLabel(loopDone) > ':' > checkMove(returnReg(), checkImm(0)) >
                         checkJump(end) >
                         checkLabel(afterBreak) > ':' >
                         checkMove(checkReg(regs[2]), checkImm(1)) >
                         checkBinaryOperation(ir::BinOp::PLUS, checkReg(regs[0]), checkReg(regs[2]), regs[3]) >
                         checkMove( // a := a + 1
                             checkReg(regs[0]), checkReg(regs[3])) >
                         checkConditionalJump(ir::RelOp::LT, regs[0], regs[1],
                                              loopStart) >
                         checkLabel(postLoop) > ':' > checkJump(loopDone) > checkLabel(end) > ':',
                     x3::eps, x3::eps, false);
    }

    SECTION("while")
    {
        auto program = checkedCompile("while 1 do break");
        OptReg regs[4];
        OptLabel loopStart, loopDone, loopNotDone, afterBreak, end;
        checkProgram(program,
                     checkLabel(loopStart) > ':' >
                         checkMove(checkReg(regs[0]), checkImm(1)) >
                         checkMove(checkReg(regs[1]), checkImm(0)) >
                         checkConditionalJump(                            // test
                             ir::RelOp::EQ, regs[0], regs[1], loopDone) > // if test equals 0 jumps to loopDone
                         checkLabel(loopNotDone) > ':' >
                         checkLabel(loopDone) > ':' >
                         checkMove(returnReg(), checkImm(0)) >
                         checkJump(end) >
                         checkLabel(afterBreak) > ':' >
                         checkJump( // jump back to loop Start
                             loopStart) > checkLabel(end) > ':',
                     x3::eps, x3::eps, false);
    }
}