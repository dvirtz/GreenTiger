#include "Test.h"

TEST_CASE_METHOD(TestFixture, "function declarations") {
  OptLabel end;
  OptIndex endIndex;

  SECTION("simple") {
    auto results = checkedCompile(R"(
let
 function f() = ()
in
end
)");
    OptLabel functionLabel, functionEnd;
    OptIndex functionLabelIndex, functionEndIndex;
    OptReg staticLink, temps[5];
    checkProgram(results,
                 checkFunctionStart(functionLabel, functionLabelIndex, temps,
                                    escapes{staticLink}), // f
                 checkMove(returnReg(), 0),               // body
                 branchToEnd(functionEnd, functionEndIndex),
                 checkFunctionExit(), checkMain(), checkMove(returnReg(), 0),
                 branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("with parameters") {
    auto results = checkedCompile(R"(
let
 function f(a:int, b:string) = ()
in
end
)");
    OptReg a, b, staticLink, temps[15];
    OptLabel functionLabel, functionEnd;
    OptIndex functionLabelIndex, functionEndIndex;
    checkProgram(results,
                 checkFunctionStart(functionLabel, functionLabelIndex, temps,
                                    escapes{staticLink}, a, b), // f
                 checkMove(returnReg(), 0),                     // body
                 branchToEnd(functionEnd, functionEndIndex),
                 checkFunctionExit(), checkMain(), checkMove(returnReg(), 0),
                 branchToEnd(end, endIndex), checkFunctionExit());
  }

  SECTION("with return value") {
    auto results = checkedCompile(R"(
let
 function f(a:int, b:string) : string = (a:=1;b)
in
end
)");
    OptReg a, b, staticLink, temps[15];
    OptLabel functionLabel, functionEnd;
    OptIndex functionLabelIndex, functionEndIndex;
    checkProgram(
      results,
      checkFunctionStart(functionLabel, functionLabelIndex, temps,
                         escapes{staticLink}, a, b),             // f
      checkInlineParameterWrite(1, a, checkImm(1), {}, {}, {b}), // body
      checkInlineParameterRead(2, b, returnReg()),
      branchToEnd(functionEnd, functionEndIndex), checkFunctionExit(),
      checkMain(), checkMove(returnReg(), 0), branchToEnd(end, endIndex),
      checkFunctionExit());
  }

  SECTION("recursive") {
    SECTION("single") {
      auto results = checkedCompile(R"(
let
 function fib(n:int) : int = (
   if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
 )
in
end
)");
      OptReg regs[18], staticLinks[3], temps[2][5], accessTemps[4][2],
        functionTemps[10];
      OptLabel functionLabel, functionEnd, trueDest[3], falseDest[3],
        joinDest[3];
      OptIndex functionLabelIndex, functionEndIndex, trueDestIndex[3],
        falseDestIndex[3], joinDestIndex[3];
      checkProgram(
        results,
        checkFunctionStart(functionLabel, functionLabelIndex, functionTemps,
                           escapes{staticLinks[0]}, regs[1]), // function label
        checkParameterAccess(1, framePointer(), accessTemps[0],
                             regs[1]), // get n from stack
        checkMove(regs[0], 0, {regs[1]}),
        checkConditionalJump(ir::RelOp::EQ, regs[1], regs[0], trueDest[0],
                             trueDestIndex[0], falseDestIndex[0]),      // n = 0
        resetReg(regs[1]), checkLabel(falseDest[0], falseDestIndex[0]), // else
        checkMove(regs[2], 1, {regs[1]}),
        checkParameterAccess(1, framePointer(), accessTemps[1], regs[1],
                             {regs[2], regs[1]}),
        checkMove(regs[15], 1, {regs[2], regs[1]}),
        checkConditionalJump(ir::RelOp::EQ, regs[1], regs[15], trueDest[1],
                             trueDestIndex[1], falseDestIndex[1]), // n = 1
        resetReg(regs[1]), checkLabel(falseDest[1], falseDestIndex[1]),
        checkMove(regs[2], 0, {regs[1]}),
        checkLabel(trueDest[1], trueDestIndex[1]),
        checkMove(regs[3], regs[2], {regs[1]}),
        checkLabel(joinDest[0], joinDestIndex[1]),
        checkMove(regs[4], 0, {regs[1], regs[3]}),
        checkConditionalJump( // test
          ir::RelOp::NE, regs[3], regs[4], trueDest[2], trueDestIndex[2],
          falseDestIndex[2], {regs[1]}),
        checkLabel(falseDest[2], falseDestIndex[2]), // else
        checkStaticLink(staticLinks[1], temps[0], {regs[1]}),
        checkParameterAccess(1, framePointer(), accessTemps[2], regs[1],
                             {regs[1], staticLinks[1]}),
        checkMove(regs[16], 1, {regs[1], staticLinks[1]}),
        checkBinaryOperation(ir::BinOp::MINUS, regs[1], regs[16], regs[5],
                             (arch == "m68k"
                                ? RegList{staticLinks[1]}
                                : RegList{regs[1], staticLinks[1]})),
        resetReg(regs[1]),
        checkCall(checkLabel(functionLabel), {returnReg(), regs[1]},
                  staticLinks[1], regs[5]),
        checkMove(regs[6], returnReg(), {regs[1]}), // fib(n-1)
        checkMove(regs[7], regs[6], {regs[1]}),
        checkStaticLink(staticLinks[2], temps[1], {regs[7], regs[1]}),
        checkParameterAccess(1, framePointer(), accessTemps[3], regs[1],
                             {regs[7], regs[1], staticLinks[2]}),
        checkMove(regs[8], 2, {regs[7], regs[1], staticLinks[2]}),
        checkBinaryOperation(ir::BinOp::MINUS, regs[1], regs[8], regs[9],
                             {regs[7], staticLinks[2]}),
        checkCall(checkLabel(functionLabel), {regs[7], returnReg()},
                  staticLinks[2], regs[9]),
        checkMove(regs[10], returnReg(), {regs[7]}), // fib(n-2)
        // add two previous calls
        checkBinaryOperation(ir::BinOp::PLUS, regs[7], regs[10], regs[11]),
        checkMove(regs[17], regs[11]),
        checkLabel(joinDest[2], joinDestIndex[2]),
        checkMove(returnReg(), regs[17]), // return result
        checkJump(functionEnd, functionEndIndex),
        checkLabel(trueDest[0], trueDestIndex[0]), // then
        checkMove(regs[3], 1), checkJump(joinDest[0], joinDestIndex[0]),
        checkLabel(trueDest[2], trueDestIndex[2]), // then
        checkInlineParameterRead(1, regs[1], regs[17]),
        checkJump(joinDest[2], joinDestIndex[2]),
        checkLabel(functionEnd, functionEndIndex), checkFunctionExit(),
        checkMain(), checkMove(returnReg(), 0), branchToEnd(end, endIndex),
        checkFunctionExit());
    }

    SECTION("mutually") {
      auto results = checkedCompile(R"(
let
 function f(x:int) = (g(1))
 function g(x:int) = (h(2))
 function h(x:int) = (f(3))
in
end
)");
      OptReg staticLinks[3], temps[3][5], regs[3], functionTemps[3][10];
      OptLabel fStart, fEnd, hStart, hEnd, gStart, gEnd;
      OptIndex fStartIndex, fEndIndex, hStartIndex, hEndIndex, gStartIndex,
        gEndIndex;
      checkProgram(results,
                   checkFunctionStart(fStart, fStartIndex, functionTemps[0],
                                      escapes{staticLinks[2]}, regs[0]),
                   checkStaticLink(staticLinks[0], temps[0]),
                   checkCall(checkLabel(gStart), {}, staticLinks[0], 1),
                   branchToEnd(fEnd, fEndIndex), checkFunctionExit(),
                   checkFunctionStart(gStart, gStartIndex, functionTemps[1],
                                      escapes{staticLinks[0]}, regs[1]),
                   checkStaticLink(staticLinks[1], temps[1]),
                   checkCall(checkLabel(hStart), {}, staticLinks[1], 2),
                   branchToEnd(gEnd, gEndIndex), checkFunctionExit(),
                   checkFunctionStart(hStart, hStartIndex, functionTemps[2],
                                      escapes{staticLinks[1]}, regs[2]),
                   checkStaticLink(staticLinks[2], temps[2]),
                   checkCall(checkLabel(fStart), {}, staticLinks[2], 3),
                   branchToEnd(hEnd, hEndIndex), checkFunctionExit(),
                   checkMain(), checkMove(returnReg(), 0),
                   branchToEnd(end, endIndex), checkFunctionExit());
    }
  }

  SECTION("nested") {
    auto results = checkedCompile(R"(
let
 function f(i: int) : int = (
   let
     function g() : int = (
       i + 2
     )
   in
     g()
   end
 )
in
 f(3)
end
)");
    OptReg regs[7], staticLinks[2], temps[2][2], accessTemps[2],
      functionTempsF[10], functionTempsG[5];
    OptLabel f, fEnd, g, gEnd;
    OptIndex fIndex, fEndIndex, gIndex, gEndIndex;
    checkProgram(
      results,
      checkFunctionStart(g, gIndex, functionTempsG, escapes{staticLinks[1]}),
      // i is fetched from f's frame
      checkMove(regs[0], -wordSize()),
      checkBinaryOperation(ir::BinOp::PLUS, framePointer(), regs[0], regs[1]),
      checkMove(checkReg(regs[2]), checkMemoryAccess(checkReg(regs[1])),
                {regs[1]}, {regs[2]}),
      checkParameterAccess(1, regs[2], accessTemps, regs[3], {}, true),
      // i+2
      checkMove(regs[4], 2, {regs[3]}),
      checkBinaryOperation(ir::BinOp::PLUS, regs[3], regs[4], regs[5]),
      checkMove(returnReg(), regs[5]), branchToEnd(gEnd, gEndIndex),
      checkFunctionExit(),
      checkFunctionStart(f, fIndex, functionTempsF, escapes{staticLinks[0]},
                         escapes{regs[6]}),
      checkStaticLink(staticLinks[1], temps[1]),
      checkCall(checkLabel(g), {}, staticLinks[1]), // g()
      branchToEnd(fEnd, fEndIndex), checkFunctionExit(), checkMain(),
      checkStaticLink(staticLinks[0], temps[0]),
      checkCall(checkLabel(f), {}, staticLinks[0], 3), // f(3)
      branchToEnd(end, endIndex), checkFunctionExit());
  }
}