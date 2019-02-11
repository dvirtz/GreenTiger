#include "Test.h"

TEST_CASE_METHOD(TestFixture, "lvalue") {
  OptLabel end;
  OptIndex endIndex;

  SECTION("identifier") {
    auto results = checkedCompile(R"(
let
  var i := 0
in
  i
end
)");

    // allocates a register and set it to 0
    // return previously allocated reg
    OptReg reg;
    checkProgram(results, checkMain(),
                 checkMove(reg, 0), // allocates a register and set it to 0
                 checkMove(returnReg(), reg), // return previously allocated reg
                 branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("field") {
    auto results = checkedCompile(R"(
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
    checkProgram(results, checkMain(),
                 // allocates a register and set it to 0
                 checkMove(regs[0], 0),
                 // set a register to the offset of i in rec
                 // relative to previously allocated reg
                 checkMemberAccess(regs[0], 0, regs[1],
                                   temps), // returns the value of i
                 checkMove(returnReg(), regs[1]), branchToEnd(end, endIndex),
                 checkFunctionExit());
  }

  SECTION("array element") {
    auto results = checkedCompile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[1]
end
)");
    OptReg regs[8], temps[4];
    checkProgram(
      results, checkMain(), checkMove(regs[0], wordSize()),
      checkMove(regs[1], 2, {regs[0]}),
      checkBinaryOperation(ir::BinOp::MUL, regs[0], regs[1], regs[2]),
      checkCall("malloc", {returnReg()}, regs[2]),
      checkMove(regs[3],
                returnReg()), // set regs[3] to result of malloc(array size),
      checkCall("initArray", {regs[3]}, 2, 3), // call initArray(array
                                       // size, array init val)
      checkMove(regs[6], regs[3]),     // move regs[3] to regs[6],
      checkMemberAccess(regs[6], 1, regs[7], temps),
      checkMove(returnReg(), regs[7]), // returns the offset of element 1 in arr
                                       // relative to regs[6]
      branchToEnd(end, endIndex), checkFunctionExit());
  }
}