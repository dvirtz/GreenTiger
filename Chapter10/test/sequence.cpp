#include "Test.h"

TEST_CASE_METHOD(TestFixture, "sequence") {
  OptLabel end;
  OptIndex endIndex;
  SECTION("empty") {
    auto results = checkedCompile("()");
    checkProgram(results, checkMain(), checkMove(returnReg(), 0),
                 branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("single") {
    auto results = checkedCompile("(42)");
    checkProgram(results, checkMain(), checkMove(returnReg(), 42),
                 branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("multiple") {
    // no-op expressions are removed
    auto results = checkedCompile(R"((1;"two";flush()))");
    OptReg temps[2], staticLink;
    OptLabel strLabel;
    OptIndex stringLabelIndex;
    checkProgram(results,
                 checkStringInit(strLabel, R"("two")", stringLabelIndex),
                 checkMain(), checkStaticLink(staticLink, temps),
                 checkCall("flush", {}, staticLink), branchToEnd(end, endIndex),
                 checkFunctionExit());
  }
}