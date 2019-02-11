#include "Test.h"

TEST_CASE_METHOD(TestFixture, "string") {
  auto str     = R"("\tHello \"World\"!\n")";
  auto results = checkedCompile(str);
  OptLabel stringLabel, end;
  OptIndex stringLabelIndex, endIndex;
  checkProgram(results, checkStringInit(stringLabel, str, stringLabelIndex), checkMain(),
               checkMove(checkReg(returnReg()), checkString(stringLabel), {}, {returnReg()}),
               branchToEnd(end, endIndex), checkFunctionExit());
}