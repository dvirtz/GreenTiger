#include "Test.h"

TEST_CASE_METHOD(TestFixture, "if then") {
  OptReg regs[4];
  OptLabel trueDest, falseDest, joinDest, end;
  OptIndex trueDestIndex, falseDestIndex, joinDestIndex, endIndex;
  SECTION("no else") {
    auto results = checkedCompile(R"(
let
 var a := 0
in
 if 1 then a := 3
end
)");
    checkProgram(results, checkMain(),
                 // every CJUMP(cond, lt , lf) is immediately followed by
                 // LABEL(lf), its "false branch."
                 checkMove(regs[0], 0), // move 0 to a
                 checkMove(regs[1], 1), checkMove(regs[2], 0, {regs[1]}),
                 checkConditionalJump( // test
                   ir::RelOp::NE, regs[1], regs[2], trueDest, trueDestIndex,
                   falseDestIndex),
                 checkLabel(falseDest, falseDestIndex),
                 checkLabel(joinDest, joinDestIndex),
                 // branches join here
                 checkMove(returnReg(), 0), // return 0
                 checkJump(end, endIndex), checkLabel(trueDest, trueDestIndex),
                 checkMove(regs[0], 3), // move 3 to a
                 checkJump(joinDest, joinDestIndex), checkLabel(end, endIndex),
                 checkFunctionExit());
  }

  SECTION("with else") {
    SECTION("both expressions are of type void") {
      auto results = checkedCompile(R"(
let
 var a := 0
in
 if 0 then a := 3 else a := 4
end
)");
      checkProgram(results, checkMain(),
                   // every CJUMP(cond, lt , lf) is immediately followed by
                   // LABEL(lf), its "false branch."
                   checkMove(regs[0], 0), // move 0 to a
                   checkMove(regs[1], 0), checkMove(regs[2], 0, {regs[1]}),
                   checkConditionalJump( // test
                     ir::RelOp::NE, regs[1], regs[2], trueDest, trueDestIndex,
                     falseDestIndex),
                   checkLabel(falseDest, falseDestIndex), // else
                   checkMove(regs[0], 4),                 // move 4 to a
                   checkLabel(joinDest, joinDestIndex),   // branches join here
                   checkMove(returnReg(), 0),             // return 0
                   checkJump(end, endIndex),
                   checkLabel(trueDest, trueDestIndex), // then
                   checkMove(regs[0], 3),               // move 3 to a
                   checkJump(joinDest, joinDestIndex),
                   checkLabel(end, endIndex), checkFunctionExit());
    }

    SECTION("both expressions are of the same type") {
      auto results = checkedCompile(R"(
let
 var a := if 1 then 2 else 3
in
 a
end
)");
      checkProgram(results, checkMain(),
                   // every CJUMP(cond, lt , lf) is immediately followed by
                   // LABEL(lf), its "false branch."
                   checkMove(regs[1], 1), checkMove(regs[2], 0, {regs[1]}),
                   checkConditionalJump( // test
                     ir::RelOp::NE, regs[1], regs[2], trueDest, trueDestIndex,
                     falseDestIndex),
                   checkLabel(falseDest, falseDestIndex), // else
                   checkMove(regs[0], 3),                 // move 3 to r
                   checkLabel(joinDest, joinDestIndex),
                   checkMove(regs[3], regs[0]),     // move r to a
                   checkMove(returnReg(), regs[3]), // return a
                   checkJump(end, endIndex),
                   checkLabel(trueDest, trueDestIndex), // then
                   checkMove(regs[0], 2),               // move 2 to r
                   checkJump(joinDest, joinDestIndex),
                   checkLabel(end, endIndex), checkFunctionExit());
    }
  }
}