#include "Test.h"

TEST_CASE_METHOD(TestFixture, "array") {
  auto results = checkedCompile(R"(
let
 type t = array of int
in
 t[2] of 3
end
)");
  OptReg regs[4];
  OptLabel end;
  OptIndex endIndex;
  checkProgram(
    results, checkMain(), checkMove(regs[0], wordSize()), checkMove(regs[1], 2, {regs[0]}),
    checkBinaryOperation(ir::BinOp::MUL, regs[0], regs[1], regs[2]),
    checkCall("malloc", {returnReg()}, regs[2]),
    checkMove(regs[3], returnReg()), // move result of malloc(array_size) to r
    checkCall("initArray", {regs[3]}, 2, 3),    // call initArray(array_size, init_val),
    checkMove(returnReg(), regs[3]), branchToEnd(end, endIndex),
    checkFunctionExit());
}