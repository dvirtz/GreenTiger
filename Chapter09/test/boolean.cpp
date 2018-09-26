#include "Test.h"

TEST_CASE("boolean") {
  OptReg regs[3];
  OptLabel trueDest, falseDest, joinDest, end;

  SECTION("and") {
    auto program = checkedCompile("2 & 3");
    // a & b is translated to if a then b else 0
    checkProgram(program,
                 // every CJUMP(cond, lt , lf) is immediately followed by
                 // LABEL(lf), its "false branch."
                 checkMove(checkReg(regs[0]), checkImm(2))
                   > checkMove(checkReg(regs[1]), checkImm(0))
                   > checkConditionalJump( // test
                       ir::RelOp::NE, regs[0], regs[1], trueDest)
                   > checkLabel(falseDest) > ':' >             // else
                   checkMove(checkReg(regs[2]), checkImm(0)) > // 0
                   checkLabel(joinDest) > ':' > // branches join here
                   checkMove(returnReg(), checkReg(regs[2])) > checkJump(end)
                   > checkLabel(trueDest) > ':' >              // then
                   checkMove(checkReg(regs[2]), checkImm(3)) > // 3
                   checkJump(joinDest) > checkLabel(end) > ':',
                 x3::eps, x3::eps, false);
  }

  SECTION("or") {
    auto program = checkedCompile("2 | 3");
    // a | b is translated to if a then 1 else b
    checkProgram(program,
                 checkMove(checkReg(regs[0]), checkImm(2))
                   > checkMove(checkReg(regs[1]), checkImm(0)) >
                   // every CJUMP(cond, lt , lf) is immediately followed by
                   // LABEL(lf), its "false branch."
                   checkConditionalJump( // test
                     ir::RelOp::NE, regs[0], regs[1], trueDest)
                   > checkLabel(falseDest) > ':' >             // else
                   checkMove(checkReg(regs[2]), checkImm(3)) > // 3
                   checkLabel(joinDest) > ':' > // branches join here
                   checkMove(returnReg(), checkReg(regs[2])) > checkJump(end)
                   > checkLabel(trueDest) > ':' >              // then
                   checkMove(checkReg(regs[2]), checkImm(1)) > // 1
                   checkJump(joinDest) > checkLabel(end) > ':',
                 x3::eps, x3::eps, false);
  }
}