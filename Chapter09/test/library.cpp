#include "Test.h"

template <typename CheckArgs = x3::eps_parser, typename CheckStrings = x3::eps_parser>
auto checkLibraryCall(const std::string &name,
                      const std::string &params,
                      const CheckArgs &checkArgs = x3::eps,
                      const boost::optional<std::string> &returnType = {},
                      const CheckStrings &checkStrings = x3::eps)
{
    OptReg v, staticLink, temps[2];
    auto program = returnType ? ("let var v : " + *returnType + " := " + name +
                                 "(" + params + ") in v end")
                              : (name + "(" + params + ")");
    auto compiled = checkedCompile(program);
    auto checkReturnValue = (x3::eps(returnType.is_initialized()) >
                             checkMove(checkReg(v), returnReg()) >
                             checkMove(returnReg(), checkReg(v))) |
                            x3::eps;
    checkProgram(compiled,
                 checkLocalCall(x3::lit(name), staticLink, temps, checkArgs) >
                     checkReturnValue,
                 checkStrings);
}

TEST_CASE("standard library")
{

    using namespace std::string_literals;
    auto emptyString = "\"\""s;

    // function print(s : string)
    SECTION("print")
    {
        OptLabel stringLabel;
        checkLibraryCall("print", emptyString, checkArg(checkString(stringLabel)), {}, checkStringInit(stringLabel, emptyString));
    }

    // function flush()
    SECTION("flush") { checkLibraryCall("flush", ""); }

    // function getchar() : string
    SECTION("getchar") { checkLibraryCall("getchar", "", x3::eps, "string"s); }

    // function ord(s: string) : int
    SECTION("ord")
    {
        OptLabel stringLabel;
        checkLibraryCall("ord", emptyString,
                         checkArg(checkString(stringLabel)), "int"s, checkStringInit(stringLabel, emptyString));
    }

    // function chr(i: int) : string
    SECTION("chr")
    {
        checkLibraryCall("chr", "0", checkArg(checkImm(0)), "string"s);
    }

    // function size(s: string) : int
    SECTION("size")
    {
        OptLabel stringLabel;
        checkLibraryCall("size", emptyString,
                         checkArg(checkString(stringLabel)), "int"s, checkStringInit(stringLabel, emptyString));
    }

    // function substring(s:string, first:int, n:int) : string
    SECTION("substring")
    {
        OptLabel stringLabel;
        checkLibraryCall("substring", emptyString + ", 1, 2",
                         checkArg(checkString(stringLabel)) > checkArg(checkImm(1)) >
                             checkArg(checkImm(2)),
                         "string"s, checkStringInit(stringLabel, emptyString));
    }

    // function concat(s1: string, s2: string) : string
    SECTION("concat")
    {
        OptLabel stringLabel[2];
        checkLibraryCall("concat", emptyString + ", " + emptyString,
                         checkArg(checkString(stringLabel[0])) > checkArg(checkString(stringLabel[1])), "string"s, checkStringInit(stringLabel[0], emptyString) > checkStringInit(stringLabel[1], emptyString));
    }

    // function not(i : integer) : integer
    SECTION("not") { checkLibraryCall("not", "3", checkArg(checkImm(3)), "int"s); }

    // function exit(i: int)
    SECTION("exit") { checkLibraryCall("exit", "5", checkArg(checkImm(5))); }
}