#include "Test.h"

TEST_CASE("record") {
  SECTION("empty") {
    auto program = checkedCompile(R"(
let
 type t = {}
in
 t{}
end
)");
    OptReg regs[2];
    checkProgram(program,
                 checkExternalCall("malloc", checkArg(0, checkImm(0)))
                   > checkMove( // move result of malloc(record_size) to
                       checkReg(regs[1]), returnReg())
                   > checkMove(returnReg(), checkReg(regs[1])));
  }

  SECTION("not empty") {
    auto program = checkedCompile(R"(
let
 type t = {a: int, b: string}
in
 t{a = 2, b = "hello"}
end
)");

    OptReg regs[4], temps[2][4];
    OptLabel stringLabel;
    checkProgram(
      program,
      checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize())))
        > checkMove(
            checkReg(regs[1]), // move result of malloc(record_size) to r
            returnReg())
        > checkMemberAccess(regs[1], 0, regs[2], temps[0])
        > checkMove(checkReg(regs[2]), // init first member with 2
                    checkImm(2))
        > checkMemberAccess(regs[1], 1, regs[3], temps[1])
        > checkMove(checkReg(regs[3]), // init second member with "hello"
                    checkString(stringLabel))
        > checkMove(returnReg(), checkReg(regs[1])),
      checkStringInit(stringLabel, R"("hello")"));
  }
}