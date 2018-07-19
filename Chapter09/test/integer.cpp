#include "Test.h"

TEST_CASE("integer")
{
    auto program = checkedCompile("42");
    checkProgram(program, checkMove(returnReg(), checkImm(42)));
}