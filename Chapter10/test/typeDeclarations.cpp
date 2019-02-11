#include "Test.h"

TEST_CASE_METHOD(TestFixture, "type declarations") {
  OptLabel end;
  OptIndex endIndex;

  SECTION("name") {
    auto results = checkedCompile(R"(
let
 type a = int
 var b : a := 0
in
end
)");
    OptReg b;
    checkProgram(results, checkMain(), checkMove(b, 0), // move 0 to b
                 checkMove(returnReg(), 0), branchToEnd(end, endIndex),
                 checkFunctionExit());
  }

  SECTION("record") {
    auto results = checkedCompile(R"(
let
 type rec = {i : int}
 var r := rec{i=2}
in
end
)");
    OptReg regs[3], temps[4];
    checkProgram(
      results, checkMain(), checkCall("malloc", {returnReg()}, wordSize()),
      checkMove(regs[0],
                returnReg()), // move result of malloc(record_size) to base reg
      checkMemberAccess(regs[0], 0, regs[1], temps, {regs[0]}),
      checkMove(regs[1], 2, {regs[0]}), // init first member with 2
      checkMove(regs[2], regs[0]),      // move base reg to r
      checkMove(returnReg(), 0), branchToEnd(end, endIndex),
      checkFunctionExit());
  }

  SECTION("array") {
    auto results = checkedCompile(R"(
let
 type arr = array of string
 var a := arr[3] of "a"
in
end
)");
    OptReg regs[5];
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkProgram(
      results, checkStringInit(stringLabel, R"("a")", stringLabelIndex),
      checkMain(), checkMove(regs[0], wordSize()),
      checkMove(regs[1], 3, {regs[0]}),
      checkBinaryOperation(ir::BinOp::MUL, regs[0], regs[1], regs[2]),
      checkCall("malloc", {returnReg()}, regs[2]),
      checkMove(regs[3],
                returnReg()), // set base reg to result of malloc(array size),
      checkCall("initArray", {regs[3]}, 3,
                stringLabel),      // call initArray(array size, array init val)
      checkMove(regs[4], regs[3]), // move base reg to a
      checkMove(returnReg(), 0), branchToEnd(end, endIndex),
      checkFunctionExit());
  }

  SECTION("recursive") {
    SECTION("single") {
      auto results = checkedCompile(R"(
let
 type intlist = {hd: int, tl: intlist}
in
 intlist{hd = 3, tl = intlist{hd = 4, tl = nil}}
end
)");
      OptReg regs[7], accessTemps[3][4], addressTemps[3];
      checkProgram(
        results, checkMain(),
        checkCall("malloc", {returnReg()}, 2 * wordSize()),
        checkMove(
          regs[0],
          returnReg()), // move result of malloc(record_size) to base reg
        checkMemberAccess(regs[0], 0, regs[1], accessTemps[0], {regs[0]}),
        checkMove(regs[1], 3, {regs[0]}), // init first member
        checkMemberAddress(regs[0], 1, regs[2], addressTemps, {regs[0]}),
        checkMove(regs[3], regs[2],
                  {regs[0], regs[3]}), // move address of second member to a register
        checkCall("malloc", {returnReg(), regs[0], regs[3]}, 2 * wordSize()),
        checkMove(regs[4], returnReg(),
                  {regs[0], regs[3]}), // move result of malloc(record_size)
                              // to a base register
        checkMemberAccess(regs[4], 0, regs[5], accessTemps[1],
                          {regs[0], regs[3], regs[4]}),
        checkMove(regs[5], 4, {regs[0], regs[3], regs[4]}), // init first member
        checkMemberAccess(regs[4], 1, regs[6], accessTemps[2],
                          {regs[0], regs[3], regs[4]}),
        checkMove(regs[6], 0,
                  {regs[0], regs[3], regs[4]}), // init second member
        checkMove(checkMemoryAccess(checkReg(regs[3])), checkReg(regs[4]),
                  {regs[3], regs[4]}, {}, false,
                  {regs[0]}), // init second member
        checkMove(returnReg(), regs[0]), branchToEnd(end, endIndex),
        checkFunctionExit());
    }

    SECTION("mutually") {
      auto results = checkedCompile(R"(
    let
     type tree = {key: int, children: treelist}
     type treelist = {hd: tree, tl: treelist}
    in
     tree{key = 1, children = treelist{hd = tree{key = 2, children = nil}, tl =
     nil}}
    end
    )");
      OptReg regs[11], accessTemps[4][4], addressTemps[2][3];
      checkProgram(
        results, checkMain(),
        checkCall("malloc", {returnReg()}, 2 * wordSize()),
        checkMove(
          regs[0],
          returnReg()), // move result of malloc(record_size) to a base register
        checkMemberAccess(regs[0], 0, regs[1], accessTemps[0], {regs[0]}),
        checkMove(regs[1], 1, {regs[0]}), // init first member
        checkMemberAddress(regs[0], 1, regs[2], addressTemps[0], {regs[0]}),
        checkMove(regs[3], regs[2],
                  {regs[0]}), // move address of second member to a register
        checkCall("malloc", {returnReg(), regs[0], regs[3]}, 2 * wordSize()),
        checkMove(
          regs[4], returnReg(),
          {regs[0],
           regs[3]}), // move result of malloc(record_size) to a base register
        checkMemberAddress(regs[4], 0, regs[5], addressTemps[1],
                           {regs[0], regs[3], regs[4]}),
        checkMove(
          regs[6], regs[5],
                  {regs[0], regs[3],
                   regs[4]}), // move address of first member to a register
        checkCall("malloc", {returnReg(), regs[0], regs[3], regs[4], regs[6]},
                  2 * wordSize()),
        checkMove(regs[7], returnReg(),
                  {regs[0], regs[3], regs[4], regs[6]}), // move result of
                                                             // malloc(record_size)
                                                             // to a register
        checkMemberAccess(regs[7], 0, regs[8], accessTemps[1],
                          {regs[0], regs[3], regs[4], regs[6], regs[7]}),
        checkMove(regs[8], 2,
          {regs[0], regs[3], regs[4], regs[6], regs[7]}), // init first member
        checkMemberAccess(regs[7], 1, regs[9], accessTemps[2],
                          {regs[0], regs[3], regs[4], regs[6], regs[7]}),
        checkMove(regs[9], 0,
          {regs[0], regs[3], regs[4], regs[6], regs[7]}), // init second member
        checkMove(checkMemoryAccess(checkReg(regs[6])), checkReg(regs[7]),
                  {regs[6], regs[7]}, {}, false,
                  {regs[0], regs[3], regs[4]}), // init first member
        checkMemberAccess(regs[4], 1, regs[10], accessTemps[3],
                          {regs[0], regs[3], regs[4]}),
        checkMove(regs[10], 0,
                  {regs[0], regs[3], regs[4]}), // init second member
        checkMove(checkMemoryAccess(checkReg(regs[3])), checkReg(regs[4]),
                  {regs[3], regs[4]}, {}, false, {regs[0]}), // init second member
        checkMove(returnReg(), regs[0]), branchToEnd(end, endIndex),
        checkFunctionExit());
    }
  }
}