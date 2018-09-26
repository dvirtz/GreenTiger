#include "Test.h"

TEST_CASE("array") {
  auto program = checkedCompile(R"(
let
 type t = array of int
in
 t[2] of 3
end
)");
  OptReg regs[4];
  checkProgram(
    program,
    checkMove(checkReg(regs[0]), checkImm(wordSize()))
      > checkMove(checkReg(regs[1]), checkImm(2))
      > checkBinaryOperation(ir::BinOp::MUL, checkReg(regs[0]),
                             checkReg(regs[1]), regs[2])
      > checkExternalCall("malloc", checkArg(0, checkReg(regs[2])))
      > checkMove( // move result of malloc(array_size) to r
          checkReg(regs[3]),
          returnReg())
      > checkExternalCall( // call initArray(array_size, init_val)
          "initArray", checkArg(0, checkImm(2)) > checkArg(1, checkImm(3)))
      > checkMove(returnReg(), checkReg(regs[3])));
}