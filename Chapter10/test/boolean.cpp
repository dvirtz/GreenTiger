#include "Test.h"

TEST_CASE_METHOD(TestFixture, "boolean") {
  OptReg regs[3];
  OptLabel trueDest, falseDest, joinDest, end;
  OptIndex trueDestIndex, falseDestIndex, joinDestIndex, endIndex;

  SECTION("and") {
    auto results = checkedCompile("2 & 3");
    // a & b is translated to if a then b else 0
    checkProgram(results, checkMain(),
                 // every CJUMP(cond, lt , lf) is immediately followed by
                 // LABEL(lf), its "false branch."
                 checkMove(regs[0], 2), checkMove(regs[1], 0, {regs[0]}),
                 checkConditionalJump( // test
                   ir::RelOp::NE, regs[0], regs[1], trueDest, trueDestIndex,
                   falseDestIndex),
                 checkLabel(falseDest, falseDestIndex), // else
                 checkMove(regs[2], 0),                 // 0
                 checkLabel(joinDest, joinDestIndex),   // branches join here
                 checkMove(returnReg(), regs[2]), checkJump(end, endIndex),
                 checkLabel(trueDest, trueDestIndex), // then
                 checkMove(regs[2], 3),               // 3
                 checkJump(joinDest, joinDestIndex), checkLabel(end, endIndex),
                 checkFunctionExit());
  }

  SECTION("or") {
    auto results = checkedCompile("2 | 3");
    // a | b is translated to if a then 1 else b
    checkProgram(results, checkMain(), checkMove(regs[0], 2),
                 checkMove(regs[1], 0, {regs[0]}),
                 // every CJUMP(cond, lt , lf) is immediately followed by
                 // LABEL(lf), its "false branch."
                 checkConditionalJump( // test
                   ir::RelOp::NE, regs[0], regs[1], trueDest, trueDestIndex,
                   falseDestIndex),
                 checkLabel(falseDest, falseDestIndex), // else
                 checkMove(regs[2], 3),                 // 3
                 checkLabel(joinDest, joinDestIndex),   // branches join here
                 checkMove(returnReg(), regs[2]), checkJump(end, endIndex),
                 checkLabel(trueDest, trueDestIndex), // then
                 checkMove(regs[2], 1),               // 1
                 checkJump(joinDest, joinDestIndex), checkLabel(end, endIndex),
                 checkFunctionExit());
  }
}