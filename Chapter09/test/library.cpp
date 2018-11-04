#include "Test.h"

template <typename CheckArgs    = x3::eps_parser,
          typename CheckStrings = x3::eps_parser>
auto checkLibraryCall(const std::string &name, const std::string &params,
                      const CheckArgs &checkArgs                     = x3::eps,
                      const boost::optional<std::string> &returnType = {},
                      const CheckStrings &checkStrings = x3::eps) {
  OptReg v, staticLink, temps[2];
  OptLabel end;
  auto program = returnType ? ("let var v : " + *returnType + " := " + name
                               + "(" + params + ") in v end")
                            : (name + "(" + params + ")");
  auto compiled = checkedCompile(program);
  auto checkReturnValue =
    (x3::eps(returnType.is_initialized()) > checkMove(checkReg(v), returnReg())
     > checkMove(returnReg(), checkReg(v)))
    | x3::eps;
  checkProgram(compiled, checkStrings, checkMain(),
               checkLocalCall(x3::lit(name), staticLink, temps, checkArgs),
               checkReturnValue, branchToEnd(end));
}

TEST_CASE("standard library") {
  using namespace std::string_literals;
  auto emptyString = "\"\""s;

  // function print(s : string)
  SECTION("print") {
    OptLabel stringLabel;
    checkLibraryCall("print", emptyString,
                     checkArg(1, checkString(stringLabel)), {},
                     checkStringInit(stringLabel, emptyString));
  }

  // function flush()
  SECTION("flush") { checkLibraryCall("flush", ""); }

  // function getchar() : string
  SECTION("getchar") { checkLibraryCall("getchar", "", x3::eps, "string"s); }

  // function ord(s: string) : int
  SECTION("ord") {
    OptLabel stringLabel;
    checkLibraryCall("ord", emptyString, checkArg(1, checkString(stringLabel)),
                     "int"s, checkStringInit(stringLabel, emptyString));
  }

  // function chr(i: int) : string
  SECTION("chr") {
    checkLibraryCall("chr", "0", checkArg(1, checkImm(0)), "string"s);
  }

  // function size(s: string) : int
  SECTION("size") {
    OptLabel stringLabel;
    checkLibraryCall("size", emptyString, checkArg(1, checkString(stringLabel)),
                     "int"s, checkStringInit(stringLabel, emptyString));
  }

  // function substring(s:string, first:int, n:int) : string
  SECTION("substring") {
    OptLabel stringLabel;
    checkLibraryCall("substring", emptyString + ", 1, 2",
                     checkArg(1, checkString(stringLabel))
                       > checkArg(2, checkImm(1)) > checkArg(3, checkImm(2)),
                     "string"s, checkStringInit(stringLabel, emptyString));
  }

  // function concat(s1: string, s2: string) : string
  SECTION("concat") {
    OptLabel stringLabel[2];
    checkLibraryCall("concat", emptyString + ", " + emptyString,
                     checkArg(1, checkString(stringLabel[0]))
                       > checkArg(2, checkString(stringLabel[1])),
                     "string"s,
                     checkStringInit(stringLabel[0], emptyString)
                       > checkStringInit(stringLabel[1], emptyString));
  }

  // function not(i : integer) : integer
  SECTION("not") {
    checkLibraryCall("not", "3", checkArg(1, checkImm(3)), "int"s);
  }

  // function exit(i: int)
  SECTION("exit") { checkLibraryCall("exit", "5", checkArg(1, checkImm(5))); }
}