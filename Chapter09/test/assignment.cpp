#include "Test.h"

TEST_CASE("assignment") {
  auto program = checkedCompile(R"(
let
 var a := 3
 var b := 0
in
 b := a
end
)");
  OptReg a, b;
  OptLabel end;
  checkProgram(program, checkMain(),
               checkMove( // move 3 to a
                 checkReg(a), checkImm(3)),
               checkMove( // move 0 to b
                 checkReg(b), checkImm(0)),
               checkMove( // move a to b
                 checkReg(b), checkReg(a)),
               checkMove( // return 0
                 returnReg(), checkImm(0)),
               branchToEnd(end));
}