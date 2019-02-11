#include "Test.h"

TEST_CASE_METHOD(TestFixture, "function call") {
  OptLabel functionLabel, functionEnd, end;
  OptIndex functionLabelIndex, functionEndIndex, endIndex;
  OptReg staticLink, temps[2];

  SECTION("with no arguments") {
    OptReg functionStartTemps[5];
    auto results = checkedCompile(R"(
let
 function f() = ()
in
 f()
end
)");

    checkProgram(
      results,
      checkFunctionStart(functionLabel, functionLabelIndex, functionStartTemps,
                         escapes{staticLink}), // function label
      checkMove(returnReg(), 0),              // body
      branchToEnd(functionEnd, functionEndIndex), checkFunctionExit(),
      checkMain(), checkStaticLink(staticLink, temps),
      checkCall(checkLabel(functionLabel), {}, staticLink), // call
      branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("with arguments") {
    OptReg a, functionStartTemps[10];
    auto results = checkedCompile(R"(
  let
   function f(a: int) = ()
  in
   f(2)
  end
  )");
    checkProgram(
      results,
      checkFunctionStart(functionLabel, functionLabelIndex, functionStartTemps,
                         escapes{staticLink}, a), // function label,
      checkMove(returnReg(), 0),                 // body
      branchToEnd(functionEnd, functionEndIndex), checkFunctionExit(),
      checkMain(), checkStaticLink(staticLink, temps),
      checkCall(checkLabel(functionLabel), {}, staticLink, 2), // call
      branchToEnd(end, endIndex), checkFunctionExit());
  }
}