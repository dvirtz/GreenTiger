#include "Test.h"

TEST_CASE_METHOD(TestFixture, "assignment") {
  auto results = checkedCompile(R"(
let
 var a := 3
 var b := 0
in
 b := a
end
)");
  OptReg a, b;
  OptLabel end;
  OptIndex endIndex;
  checkProgram(results, checkMain(), checkMove(a, 3), // move 3 to a
               checkMove(b, 0, {a}),                       // move 0 to b
               checkMove(b, a),                       // move a to b
               checkMove(returnReg(), 0),             // return 0
               branchToEnd(end, endIndex), checkFunctionExit());
}