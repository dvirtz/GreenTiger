#include "Test.h"

class LibraryTestFixture : public TestFixture {
protected:
  using TestFixture::checkCall;

  template <typename... Args, size_t... I>
  auto checkCall(const std::string &name, OptReg &staticLink, bool hasReturn,
                 const std::tuple<Args...> &args, std::index_sequence<I...>) {
    const auto liveRegisters = hasReturn ? RegList{returnReg()} : RegList{};
    return checkCall(x3::lit(name), liveRegisters, staticLink, std::get<I>(args)...);
  }

  template <typename ArgsTuple    = std::tuple<>,
            typename CheckStrings = x3::eps_parser>
  auto checkLibraryCall(const std::string &name, const std::string &params = {},
                        const ArgsTuple &args                          = {},
                        const boost::optional<std::string> &returnType = {},
                        const CheckStrings &checkStrings = x3::eps) {
    OptReg v, staticLink, temps[2];
    OptLabel end;
    OptIndex endIndex;
    auto program = returnType ? ("let var v : " + *returnType + " := " + name
                                 + "(" + params + ") in v end")
                              : (name + "(" + params + ")");
    auto checkReturnValue =
      (x3::eps(returnType.is_initialized()) > checkMove(v, returnReg())
       > checkMove(returnReg(), v))
      | x3::eps;
    auto results = checkedCompile(program);
    checkProgram(
      results, checkStrings, checkMain(), checkStaticLink(staticLink, temps),
      checkCall(name, staticLink, returnType.is_initialized(), args,
                std::make_index_sequence<std::tuple_size<ArgsTuple>::value>()),
      checkReturnValue, branchToEnd(end, endIndex), checkFunctionExit());
  }
};

TEST_CASE_METHOD(LibraryTestFixture, "standard library") {
  using namespace std::string_literals;
  auto emptyString = "\"\""s;

  // function print(s : string)
  SECTION("print") {
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkLibraryCall(
      "print", emptyString, std::forward_as_tuple(stringLabel), {},
      checkStringInit(stringLabel, emptyString, stringLabelIndex));
  }

  // function flush()
  SECTION("flush") { checkLibraryCall("flush"); }

  // function getchar() : string
  SECTION("getchar") { checkLibraryCall("getchar", {}, {}, "string"s); }

  // function ord(s: string) : int
  SECTION("ord") {
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkLibraryCall(
      "ord", emptyString, std::forward_as_tuple(stringLabel), "int"s,
      checkStringInit(stringLabel, emptyString, stringLabelIndex));
  }

  // function chr(i: int) : string
  SECTION("chr") {
    checkLibraryCall("chr", "0", std::forward_as_tuple(0), "string"s);
  }

  // function size(s: string) : int
  SECTION("size") {
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkLibraryCall(
      "size", emptyString, std::forward_as_tuple(stringLabel), "int"s,
      checkStringInit(stringLabel, emptyString, stringLabelIndex));
  }

  // function substring(s:string, first:int, n:int) : string
  SECTION("substring") {
    OptLabel stringLabel;
    OptIndex stringLabelIndex;
    checkLibraryCall(
      "substring", emptyString + ", 1, 2",
      std::forward_as_tuple(stringLabel, 1, 2), "string"s,
      checkStringInit(stringLabel, emptyString, stringLabelIndex));
  }

  // function concat(s1: string, s2: string) : string
  SECTION("concat") {
    OptLabel stringLabel[2];
    OptIndex stringLabelIndex[2];
    checkLibraryCall(
      "concat", emptyString + ", " + emptyString,
      std::forward_as_tuple(stringLabel[0], stringLabel[1]), "string"s,
      checkStringInit(stringLabel[0], emptyString, stringLabelIndex[0])
        > checkStringInit(stringLabel[1], emptyString, stringLabelIndex[1]));
  }

  // function not(i : integer) : integer
  SECTION("not") {
    checkLibraryCall("not", "3", std::forward_as_tuple(3), "int"s);
  }

  // function exit(i: int)
  SECTION("exit") { checkLibraryCall("exit", "5", std::forward_as_tuple(5)); }
}