#include "Test.h"

TEST_CASE("lvalue") {
  SECTION("identifier") {
    auto program = checkedCompile(R"(
let
  var i := 0
in
  i
end
)");

    // allocates a register and set it to 0
    // return previously allocated reg
    OptReg reg;
    OptLabel end;
    checkProgram(program, checkMain(),
                 checkMove( // allocates a register and set it to 0
                   checkReg(reg), checkImm(0)),
                 checkMove( // return previously allocated reg
                   returnReg(), checkReg(reg)),
                 branchToEnd(end));
  }

  SECTION("field") {
    auto program = checkedCompile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.i
end
)");

    // allocates a register and set it to 0
    // returns the offset of i in rec relative to
    // previously allocated reg
    OptReg regs[2], temps[4];
    OptLabel end;
    checkProgram(program, checkMain(),
                 // allocates a register and set it to 0
                 checkMove(checkReg(regs[0]), checkImm(0)),
                 // set a register to the offset of i in rec
                 // relative to previously allocated reg
                 checkMemberAccess(regs[0], 0, regs[1],
                                   temps), // returns the value of i
                 checkMove(returnReg(), checkReg(regs[1])),
                 branchToEnd(end));
  }

  SECTION("array element") {
    auto program = checkedCompile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[1]
end
)");
    OptReg regs[8], temps[4];
    OptLabel end;
    checkProgram(
      program, checkMain(), checkMove(checkReg(regs[0]), checkImm(wordSize())),
      checkMove(checkReg(regs[1]), checkImm(2)),
      checkBinaryOperation(ir::BinOp::MUL, checkReg(regs[0]), checkReg(regs[1]),
                           regs[2]),
      checkCall("malloc", checkArg(0, checkReg(regs[2]))),
      checkMove( // set regs[3] to result of malloc(array size),
        checkReg(regs[3]), returnReg()),
      checkCall( // call initArray(array
                         // size, array init val)
        "initArray", checkArg(0, checkImm(2)) > checkArg(1, checkImm(3))),
      checkMove(checkReg(regs[6]), // move regs[3] to regs[6],
                checkReg(regs[3])),
      checkMemberAccess(regs[6], 1, regs[7], temps),
      checkMove( // returns the offset of element 1 in arr
                 // relative to regs[6]
        returnReg(), checkReg(regs[7])),
      branchToEnd(end));
  }
}