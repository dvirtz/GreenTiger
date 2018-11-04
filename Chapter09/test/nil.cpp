#include "Test.h"

TEST_CASE("nil") {
  OptLabel end;
  auto program = checkedCompile("nil");
  checkProgram(program, checkMain(), checkMove(returnReg(), checkImm(0)),
               branchToEnd(end));
}