#include "Test.h"

TEST_CASE("assignment")
{
    auto program = checkedCompile(R"(
let
 var a := 3
 var b := 0
in
 b := a
end
)");
    OptReg a, b;
    checkProgram(program, checkMove( // move 3 to a
                              checkReg(a), checkImm(3)) >
                              checkMove( // move 0 to b
                                  checkReg(b), checkImm(0)) >
                              checkMove( // move b to a
                                  checkReg(b), checkReg(a)) >
                              checkMove( // return 0
                                  returnReg(), checkImm(0)));
}