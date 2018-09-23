#include "Test.h"

TEST_CASE("type declarations")
{
    SECTION("name")
    {
        auto program = checkedCompile(R"(
let
 type a = int
 var b : a := 0
in
end
)");
        OptReg b;
        checkProgram(program, checkMove( // move 0 to b
                                  checkReg(b), checkImm(0)) >
                                  checkMove(returnReg(), checkImm(0)));
    }

    SECTION("record")
    {
        auto program = checkedCompile(R"(
let
 type rec = {i : int}
 var r := rec{i=2}
in
end
)");
        OptReg regs[3], temps[4];
        checkProgram(program,
                     checkExternalCall("malloc", checkArg(0, checkImm(wordSize()))) >
                         checkMove( // move result of malloc(record_size) to base reg

                             checkReg(regs[0]), returnReg()) >
                         checkMemberAccess(regs[0], 0, regs[1], temps) >
                         checkMove( // init first member with 2
                             checkReg(regs[1]), checkImm(2)) >
                         checkMove( // move base reg to r
                             checkReg(regs[2]), checkReg(regs[0])) >
                         checkMove(returnReg(), checkImm(0)));
    }

    SECTION("array")
    {
        auto program = checkedCompile(R"(
let
 type arr = array of string
 var a := arr[3] of "a"
in
end
)");
        OptReg regs[5];
        OptLabel stringLabel;
        checkProgram(program,
                     checkMove(checkReg(regs[0]), checkImm(wordSize())) >
                         checkMove(checkReg(regs[1]), checkImm(3)) >
                         checkBinaryOperation(ir::BinOp::MUL, checkReg(regs[0]), checkReg(regs[1]), regs[2]) >
                         checkExternalCall("malloc", checkArg(0, checkReg(regs[2]))) >
                         checkMove( // set base reg to result of malloc(array size),
                             checkReg(regs[3]), returnReg()) >
                         checkExternalCall( // call initArray(array size, array init val)
                             "initArray", checkArg(0, checkImm(3)) > checkArg(1, checkString(stringLabel))) >
                         checkMove(checkReg(regs[4]), // move base reg to a
                                   checkReg(regs[3])) >
                         checkMove(returnReg(), checkImm(0)),
                     checkStringInit(stringLabel, R"("a")"));
    }

    SECTION("recursive")
    {
        SECTION("single")
        {
            auto program = checkedCompile(R"(
let
 type intlist = {hd: int, tl: intlist}
in
 intlist{hd = 3, tl = intlist{hd = 4, tl = nil}}
end
)");
            OptReg regs[7], accessTemps[3][4], addressTemps[3];
            checkProgram(program,
                         checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize()))) >
                             checkMove( // move result of malloc(record_size) to base reg
                                 checkReg(regs[0]), returnReg()) >
                             checkMemberAccess(regs[0], 0, regs[1], accessTemps[0]) >
                             checkMove( // init first member
                                 checkReg(regs[1]), checkImm(3)) >
                             checkMemberAddress(regs[0], 1, regs[2], addressTemps) >
                             checkMove( // move address of second member to a register
                                 checkReg(regs[3]), checkReg(regs[2])) >
                             checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize()))) >
                             checkMove( // move result of malloc(record_size)
                                        // to a base register
                                 checkReg(regs[4]), returnReg()) >
                             checkMemberAccess(regs[4], 0, regs[5], accessTemps[1]) >
                             checkMove( // init first member
                                 checkReg(regs[5]), checkImm(4)) >
                             checkMemberAccess(regs[4], 1, regs[6], accessTemps[2]) >
                             checkMove( // init second member
                                 checkReg(regs[6]), checkImm(0)) >
                             checkMove( // init second member
                                 checkMemoryAccess(checkReg(regs[3])), checkReg(regs[4])) >
                             checkMove(returnReg(), checkReg(regs[0])));
        }

        SECTION("mutually")
        {
            auto program = checkedCompile(R"(
    let
     type tree = {key: int, children: treelist}
     type treelist = {hd: tree, tl: treelist}
    in
     tree{key = 1, children = treelist{hd = tree{key = 2, children = nil}, tl =
     nil}}
    end
    )");
            OptReg regs[11], accessTemps[4][4], addressTemps[2][3];
            checkProgram(program,
                         checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize()))) >
                             checkMove( // move result of malloc(record_size) to a base register
                                 checkReg(regs[0]), returnReg()) >
                             checkMemberAccess(regs[0], 0, regs[1], accessTemps[0]) >
                             checkMove( // init first member
                                 checkReg(regs[1]), checkImm(1)) >
                             checkMemberAddress(regs[0], 1, regs[2], addressTemps[0]) >
                             checkMove( // move address of second member to a register
                                 checkReg(regs[3]), checkReg(regs[2])) >
                             checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize()))) >
                             checkMove( // move result of malloc(record_size) to a base register
                                 checkReg(regs[4]), returnReg()) >
                             checkMemberAddress(regs[4], 0, regs[5], addressTemps[1]) >
                             checkMove( // move address of first member to a register
                                 checkReg(regs[6]), checkReg(regs[5])) >
                             checkExternalCall("malloc", checkArg(0, checkImm(2 * wordSize()))) >
                             checkMove( // move result of
                                        // malloc(record_size) to
                                        // a register
                                 checkReg(regs[7]), returnReg()) >
                             checkMemberAccess(regs[7], 0, regs[8], accessTemps[1]) >
                             checkMove( // init first member
                                 checkReg(regs[8]), checkImm(2)) >
                             checkMemberAccess(regs[7], 1, regs[9], accessTemps[2]) >
                             checkMove( // init second member
                                 checkReg(regs[9]), checkImm(0)) >
                             checkMove( // init first member
                                 checkMemoryAccess(checkReg(regs[6])), checkReg(regs[7])) >
                             checkMemberAccess(regs[4], 1, regs[10], accessTemps[3]) >
                             checkMove( // init second member
                                 checkReg(regs[10]), checkImm(0)) >
                             checkMove( // init second member
                                 checkMemoryAccess(checkReg(regs[3])), checkReg(regs[4])) >
                             checkMove(returnReg(), checkReg(regs[0])));
        }
    }
}