#include "Test.h"

TEST_CASE("function call")
{
    OptLabel functionLabel, functionEnd;
    OptReg temps[2];
    OptReg staticLink;

    SECTION("with no arguments")
    {
        auto program = checkedCompile(R"(
let
 function f() = ()
in
 f()
end
)");

        checkProgram(program,
                     checkLocalCall( // call
                         checkLabel(functionLabel), staticLink, temps),
                     x3::eps,
                     checkLabel(functionLabel) > ':' > // function label
                         checkMove(                    // body
                             returnReg(), checkImm(0)) > branchToEnd(functionEnd));
    }

    SECTION("with arguments")
    {
        auto program = checkedCompile(R"(
  let
   function f(a: int) = ()
  in
   f(2)
  end
  )");
        checkProgram(program, checkLocalCall( // call
                                  checkLabel(functionLabel), staticLink, temps, checkArg(checkImm(2))),
                     x3::eps,
                     checkLabel(functionLabel) > ':' > // function label,
                         checkMove(                    // body
                             returnReg(), checkImm(0)) >
                         branchToEnd(functionEnd));
    }
}