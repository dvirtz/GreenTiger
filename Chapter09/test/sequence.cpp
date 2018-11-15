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
    OptReg temps[2];
    OptLabel strLabel;
    OptReg staticLink;
    checkProgram(program, checkStringInit(strLabel, R"("two")"), checkMain(),
                 checkStaticLink(staticLink, temps),
                 checkCall("flush", checkArg(0, checkReg(staticLink))),
                 branchToEnd(end));
  }
}