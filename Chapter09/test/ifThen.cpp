#include "Test.h"

TEST_CASE("if then")
{
    OptReg regs[4];
    OptLabel trueDest, falseDest, joinDest, end;
    SECTION("no else")
    {
        auto program = checkedCompile(R"(
let
 var a := 0
in
 if 1 then a := 3
end
)");
        checkProgram(program,
                     // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf),
                     // its "false branch."
                     checkMove( // move 0 to a
                         checkReg(regs[0]), checkImm(0)) >
                         checkMove(checkReg(regs[1]), checkImm(1)) >
                         checkMove(checkReg(regs[2]), checkImm(0)) >
                         checkConditionalJump( // test
                             ir::RelOp::NE, regs[1], regs[2], trueDest) >
                         checkLabel(falseDest) > ':' >
                         checkLabel(joinDest) > ':' > // branches join here
                         checkMove(                   // return 0
                             returnReg(), checkImm(0)) >
                         checkJump(end) >
                         checkLabel(trueDest) > ':' >
                         checkMove( // move 3 to a
                             checkReg(regs[0]), checkImm(3)) >
                         checkJump(joinDest) >
                         checkLabel(end) > ':',
                     x3::eps, x3::eps, false);
    }

    SECTION("with else")
    {
        SECTION("both expressions are of type void")
        {
            auto program = checkedCompile(R"(
let
 var a := 0
in
 if 0 then a := 3 else a := 4
end
)");
            checkProgram(program,
                         // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf),
                         // its "false branch."
                         checkMove( // move 0 to a
                             checkReg(regs[0]), checkImm(0)) >
                             checkMove(checkReg(regs[1]), checkImm(0)) >
                             checkMove(checkReg(regs[2]), checkImm(0)) >
                             checkConditionalJump( // test
                                 ir::RelOp::NE, regs[1], regs[2], trueDest) >
                             checkLabel(falseDest) > ':' > // else
                             checkMove(checkReg(regs[0]),
                                       checkImm(4)) >     // move 4 to a
                             checkLabel(joinDest) > ':' > // branches join here
                             checkMove(                   // return 0
                                 returnReg(), checkImm(0)) >
                             checkJump(end) >
                             checkLabel(trueDest) > ':' > // then
                             checkMove(checkReg(regs[0]),
                                       checkImm(3)) > // move 3 to a
                             checkJump(joinDest) >
                             checkLabel(end) > ':',
                         x3::eps, x3::eps, false);
        }

        SECTION("both expressions are of the same type")
        {
            auto program = checkedCompile(R"(
let
 var a := if 1 then 2 else 3
in
 a
end
)");
            checkProgram(program,
                         // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf),
                         // its "false branch."
                         checkMove(checkReg(regs[1]), checkImm(1)) >
                             checkMove(checkReg(regs[2]), checkImm(0)) >
                             checkConditionalJump( // test
                                 ir::RelOp::NE, regs[1], regs[2], trueDest) >
                             checkLabel(falseDest) > ':' > // else
                             checkMove(checkReg(regs[0]),
                                       checkImm(3)) > // move 3 to r
                             checkLabel(joinDest) > ':' >
                             checkMove( // move r to a
                                 checkReg(regs[3]), checkReg(regs[0])) >
                             checkMove( // return a
                                 returnReg(), checkReg(regs[3])) >
                             checkJump(end) >
                             checkLabel(trueDest) > ':' > // then
                             checkMove(checkReg(regs[0]),
                                       checkImm(2)) > // move 2 to r
                             checkJump(joinDest) >
                             checkLabel(end) > ':',
                         x3::eps, x3::eps, false);
        }
    }
}