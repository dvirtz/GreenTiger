#include "Test.h"

TEST_CASE("sequence")
{
    SECTION("empty")
    {
        auto program = checkedCompile("()");
        checkProgram(program, checkMove(returnReg(), checkImm(0)));
    }

    SECTION("single")
    {
        auto program = checkedCompile("(42)");
        checkProgram(program, checkMove(returnReg(), checkImm(42)));
    }

    SECTION("multiple")
    {
        // no-op expressions are removed
        auto program = checkedCompile(R"((1;"two";flush()))");
        OptReg regs[2];
        OptLabel strLabel;
        OptReg staticLink;
        checkProgram(program,
                     checkLocalCall(x3::lit("flush"), staticLink, regs),
                     checkStringInit(strLabel, R"("two")"));
    }
}