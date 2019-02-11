#include "Test.h"

TEST_CASE_METHOD(TestFixture, "record") {
  OptLabel end;
  OptIndex endIndex;

  SECTION("empty") {
    auto results = checkedCompile(R"(
let
 type t = {}
in
 t{}
end
)");
    OptReg regs[2];
    checkProgram(
      results, checkMain(), checkCall("malloc", {returnReg()}, 0),
      checkMove(regs[1], returnReg()), // move result of malloc(record_size) to
      checkMove(returnReg(), regs[1]), branchToEnd(end, endIndex),
      checkFunctionExit());
  }

  SECTION("not empty") {
    auto results = checkedCompile(R"(
let
 type t = {a: int, b: string}
in
 t{a = 2, b = "hello"}
end
)");

    OptReg regs[4], temps[2][4];
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkProgram(
      results, checkStringInit(stringLabel, R"("hello")", stringLabelIndex), checkMain(),
      checkCall("malloc", {returnReg()}, 2 * wordSize()),
      checkMove(regs[1], returnReg()),// move result of malloc(record_size) to r
      checkMemberAccess(regs[1], 0, regs[2], temps[0], {regs[1]}),
      checkMove(regs[2], 2, {regs[1]}), // init first member with 2
      checkMemberAccess(regs[1], 1, regs[3], temps[1], {regs[1]}),
      checkMove(checkReg(regs[3]), checkString(stringLabel), {}, {regs[3]}, false,
                {regs[1]}), // init second member with "hello"
      checkMove(returnReg(), regs[1]),
      branchToEnd(end, endIndex), checkFunctionExit());
  }
  }