#include "Test.h"

TEST_CASE("string") {
  auto str     = R"("\tHello \"World\"!\n")";
  auto program = checkedCompile(str);
  OptLabel stringLabel, end;
  checkProgram(program, checkStringInit(stringLabel, str), checkMain(),
               checkMove(returnReg(), checkString(stringLabel)),
               branchToEnd(end));
}