#include "Test.h"

TEST_CASE("function declarations") {
  SECTION("simple") {
    auto program = checkedCompile(R"(
let
 function f() = ()
in
end
)");
    OptLabel functionLabel, functionEnd;
    checkProgram(program, checkMove(returnReg(), checkImm(0)), x3::eps,
                 checkLabel(functionLabel) > ':' > // function label
                   checkMove(                      // body
                     returnReg(), checkImm(0))
                   > branchToEnd(functionEnd));
  }

  SECTION("with parameters") {
    auto program = checkedCompile(R"(
let
 function f(a:int, b:string) = ()
in
end
)");
    OptLabel functionLabel, functionEnd;
    checkProgram(program, checkMove(returnReg(), checkImm(0)), x3::eps,
                 checkLabel(functionLabel) > ':' > // function label
                   checkMove(                      // body
                     returnReg(), checkImm(0))
                   > branchToEnd(functionEnd));
  }

  SECTION("with return value") {
    auto program = checkedCompile(R"(
let
 function f(a:int, b:string) : string = (a:=1;b)
in
end
)");
    OptReg a, b;
    OptLabel functionLabel, functionEnd;
    checkProgram(program, checkMove(returnReg(), checkImm(0)), x3::eps,
                 checkLabel(functionLabel) > ':' > // function label
                   checkMove(                      // body
                     checkInlineParameterAccess(0, a), checkImm(1))
                   > checkMove(returnReg(), checkInlineParameterAccess(1, b))
                   > branchToEnd(functionEnd));
  }

  SECTION("recursive") {
    SECTION("single") {
      auto program = checkedCompile(R"(
let
 function fib(n:int) : int = (
   if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
 )
in
end
)");
      OptReg regs[18], staticLinks[2][2], temps[2][4], accessTemps[4][2];
      OptLabel functionLabel, functionEnd, trueDest[3], falseDest[3],
        joinDest[3];
      checkProgram(
        program, checkMove(returnReg(), checkImm(0)), x3::eps,
        checkLabel(functionLabel) > ':' > // function label
                                          // get n from stack
          checkParameterAcess(0, framePointer(), accessTemps[0], regs[1])
          > checkMove(checkReg(regs[0]), checkImm(0))
          > checkConditionalJump( // n = 0
              ir::RelOp::EQ, regs[1], regs[0], trueDest[0])
          > checkLabel( // else
              falseDest[0])
          > ':' > checkMove(checkReg(regs[2]), checkImm(1))
          > checkParameterAcess(0, framePointer(), accessTemps[1], regs[12])
          > checkMove(checkReg(regs[15]), checkImm(1))
          > checkConditionalJump( // n = 1
              ir::RelOp::EQ, regs[12], regs[15], trueDest[1])
          > checkLabel(falseDest[1]) > ':'
          > checkMove(checkReg(regs[2]), checkImm(0)) > checkLabel(trueDest[1])
          > ':' > checkMove(checkReg(regs[3]), checkReg(regs[2]))
          > checkLabel(joinDest[0]) > ':'
          > checkMove(checkReg(regs[4]), checkImm(0))
          > checkConditionalJump( // test
              ir::RelOp::NE, regs[3], regs[4], trueDest[2])
          > checkLabel( // else
              falseDest[2])
          > ':' > checkStaticLink(staticLinks[0], temps[0])
          > checkParameterAcess(0, framePointer(), accessTemps[2], regs[13])
          > checkMove(checkReg(regs[16]), checkImm(1))
          > checkBinaryOperation(ir::BinOp::MINUS, checkReg(regs[13]),
                                 checkReg(regs[16]), regs[5])
          > checkCall(checkLabel(functionLabel),
                      checkArg(0, checkReg(staticLinks[0][0]))
                        > checkArg(1, checkReg(regs[5])))
          > checkMove( // fib(n-1)
              checkReg(regs[6]), returnReg())
          > checkMove(checkReg(regs[7]), checkReg(regs[6]))
          > checkStaticLink(staticLinks[1], temps[1])
          > checkParameterAcess(0, framePointer(), accessTemps[3], regs[14])
          > checkMove(checkReg(regs[8]), checkImm(2))
          > checkBinaryOperation(ir::BinOp::MINUS, checkReg(regs[14]),
                                 checkReg(regs[8]), regs[9])
          > checkCall(checkLabel(functionLabel),
                      checkArg(0, checkReg(staticLinks[1][0]))
                        > checkArg(1, checkReg(regs[9])))
          > checkMove( // fib(n-2)
              checkReg(regs[10]), returnReg())
          >
          // add two previous calls
          checkBinaryOperation(ir::BinOp::PLUS, checkReg(regs[7]),
                               checkReg(regs[10]), regs[11])
          > checkMove(checkReg(regs[17]), checkReg(regs[11]))
          > checkLabel(joinDest[2]) > ':' > checkMove( // return result
                                              returnReg(), checkReg(regs[17]))
          > checkJump(functionEnd) > checkLabel( // then
                                       trueDest[0])
          > ':' > checkMove(checkReg(regs[3]), checkImm(1))
          > checkJump(joinDest[0]) > checkLabel( // then
                                       trueDest[2])
          > ':' > checkMove(checkReg(regs[17]),
                            checkInlineParameterAccess(0, regs[1]))
          > checkJump(joinDest[2]) > checkLabel(functionEnd) > ':');
    }

    SECTION("mutually") {
      auto program = checkedCompile(R"(
let
 function f(x:int) = (g(1))
 function g(x:int) = (h(2))
 function h(x:int) = (f(3))
in
end
)");
      OptReg staticLinks[3][2], temps[3][4];
      OptLabel fStart, fEnd, hStart, hEnd, gStart, gEnd;
      checkProgram(program, checkMove(returnReg(), checkImm(0)), x3::eps,
                   checkLabel(fStart) > ':'
                     > checkStaticLink(staticLinks[0], temps[0])
                     > checkCall(checkLabel(gStart),
                                 checkArg(0, checkReg(staticLinks[0][0]))
                                   > checkArg(1, checkImm(1)))
                     > branchToEnd(fEnd) > checkLabel(gStart) > ':'
                     > checkStaticLink(staticLinks[1], temps[1])
                     > checkCall(checkLabel(hStart),
                                 checkArg(0, checkReg(staticLinks[1][0]))
                                   > checkArg(1, checkImm(2)))
                     > branchToEnd(gEnd) > checkLabel(hStart) > ':'
                     > checkStaticLink(staticLinks[2], temps[2])
                     > checkCall(checkLabel(fStart),
                                 checkArg(0, checkReg(staticLinks[2][0]))
                                   > checkArg(1, checkImm(3)))
                     > branchToEnd(hEnd));
    }
  }

  SECTION("nested") {
    auto program = checkedCompile(R"(
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
    OptReg regs[6], staticLinks[2], temps[2][2], accessTemps[2];
    OptLabel f, fEnd, g, gEnd;
    checkProgram(
      program,
      checkLocalCall( // f(3)
        checkLabel(f), staticLinks[0], temps[0], checkArg(1, checkImm(3))),
      x3::eps,
      checkLabel(g) > ':' >
        // i is fetched from f's frame
        checkMove(checkReg(regs[0]), checkImm(-wordSize()))
        > checkBinaryOperation(ir::BinOp::PLUS, framePointer(),
                               checkReg(regs[0]), regs[1])
        > checkMove(checkReg(regs[2]), checkMemoryAccess(checkReg(regs[1])))
        > checkParameterAcess(0, checkReg(regs[2]), accessTemps, regs[3], true)
        >
        // i+2
        checkMove(checkReg(regs[4]), checkImm(2))
        > checkBinaryOperation(ir::BinOp::PLUS, checkReg(regs[3]),
                               checkReg(regs[4]), regs[5])
        > checkMove(returnReg(), checkReg(regs[5])) > branchToEnd(gEnd)
        > checkLabel(f) > ':'
        > checkLocalCall(checkLabel(g), staticLinks[1], temps[1])
        > branchToEnd(fEnd));
  }
}