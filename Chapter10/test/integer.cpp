#include "Test.h"

TEST_CASE_METHOD(TestFixture, "integer") {
  OptLabel end;
  OptIndex endIndex;
  auto results = checkedCompile("42");
  checkProgram(results, checkMain(), checkMove(returnReg(), 42),
               branchToEnd(end, endIndex), checkFunctionExit());
}