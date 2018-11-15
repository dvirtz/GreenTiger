#include "Test.h"

TEST_CASE("function call") {
  OptLabel functionLabel, functionEnd, end;
  OptReg temps[2];
  OptReg staticLink;

  SECTION("with no arguments") {
    auto program = checkedCompile(R"(
let
 function f() = ()
in
 f()
end
)");

    checkProgram(program, checkLabel(functionLabel), ':', // function label
                 checkMove(                               // body
                   returnReg(), checkImm(0)),
                 branchToEnd(functionEnd), checkMain(),
                 checkStaticLink(staticLink, temps),
                 checkCall(//call
                   checkLabel(functionLabel), checkArg(0, checkReg(staticLink))),
                 branchToEnd(end));
  }

  SECTION("with arguments") {
    auto program = checkedCompile(R"(
  let
   function f(a: int) = ()
  in
   f(2)
  end
  )");
    checkProgram(
      program, checkLabel(functionLabel), ':', // function label,
      checkMove(                               // body
        returnReg(), checkImm(0)),
      branchToEnd(functionEnd), checkMain(),
      checkStaticLink(staticLink, temps),
      checkCall( // call
        checkLabel(functionLabel), checkArg(0, checkReg(staticLink)) > checkArg(1, checkImm(2))),
      branchToEnd(end));
  }
}