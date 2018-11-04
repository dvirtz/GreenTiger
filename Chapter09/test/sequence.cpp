#include "Test.h"

TEST_CASE("sequence") {
  OptLabel end;
  SECTION("empty") {
    auto program = checkedCompile("()");
    checkProgram(program, checkMain(), checkMove(returnReg(), checkImm(0)),
                 branchToEnd(end));
  }

  SECTION("single") {
    auto program = checkedCompile("(42)");
    checkProgram(program, checkMain(), checkMove(returnReg(), checkImm(42)),
                 branchToEnd(end));
  }

  SECTION("multiple") {
    // no-op expressions are removed
    auto program = checkedCompile(R"((1;"two";flush()))");
    OptReg regs[2];
    OptLabel strLabel;
    OptReg staticLink;
    checkProgram(program, checkStringInit(strLabel, R"("two")"), checkMain(),
                 checkLocalCall(x3::lit("flush"), staticLink, regs),
                 branchToEnd(end));
  }
}