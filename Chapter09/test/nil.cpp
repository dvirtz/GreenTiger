#include "Test.h"

TEST_CASE("nil")
{
    auto program = checkedCompile("nil");
    checkProgram(program, checkMove(returnReg(), checkImm(0)));
}