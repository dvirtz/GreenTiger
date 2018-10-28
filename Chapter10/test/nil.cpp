#include "Test.h"

TEST_CASE_METHOD(TestFixture, "nil") {
  OptReg rv = returnReg();
  OptLabel end;
  OptIndex endIndex;
  auto results  = checkedCompile("nil");
  checkProgram(results, checkMain(), checkMove(rv, 0), branchToEnd(end, endIndex),
    checkFunctionExit());
  checkInterference(results.m_interferenceGraphs);
}