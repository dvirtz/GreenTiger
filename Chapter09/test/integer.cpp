#include "Test.h"

TEST_CASE("integer") {
  OptLabel end;
  auto program = checkedCompile("42");
  checkProgram(program, checkMain(), checkMove(returnReg(), checkImm(42)),
               branchToEnd(end));
}