#include "Test.h"

TEST_CASE("string")
{
    auto str = R"("\tHello \"World\"!\n")";
    auto program = checkedCompile(str);
    OptLabel stringLabel;
    checkProgram(program, checkMove(returnReg(), checkString(stringLabel)),
                 checkStringInit(stringLabel, str));
}