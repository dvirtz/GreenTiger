#include "Program.h"
#include "warning_suppress.h"
#include <array>
#include <boost/optional/optional_io.hpp>
MSC_DIAG_OFF(4496 4459 4127)
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
MSC_DIAG_ON()
#include <catch/catch.hpp>
#include <boost/format.hpp>
#include <gsl/span>

extern std::string arch;

namespace x3 = boost::spirit::x3;
namespace ir = tiger::ir;
namespace temp = tiger::temp;
using OptReg = boost::optional<temp::Register>;
using OptLabel = boost::optional<temp::Label>;

struct error_handler
{
    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator &/* first */, Iterator const &/* last */,
                                      Exception const &x,
                                      Context const &context)
    {
        auto &error_handler = x3::get<x3::error_handler_tag>(context);
        std::string message = "Error! Expecting: " + x.which() + " here:";
        error_handler(x.where(), message);
        return x3::error_handler_result::fail;
    }
};

inline auto returnReg()
{
    auto const r = x3::rule<struct return_reg>{"return reg"} =
        (arch == "m68k") ? x3::lit("D0") : x3::lit("EAX");
    return r;
}

inline auto wordSize()
{
    if (arch == "m68k")
    {
        return 4;
    }

    return 8;
}

inline auto framePointer()
{
    auto const r = x3::rule<struct frame_pointer>{"frame pointer"} =
        (arch == "m68k") ? x3::lit("A6") : x3::lit("EBP");
    return r;
}

inline auto checkedCompile(const std::string &string)
{
    auto res = tiger::compile(arch, string);
    REQUIRE(res);
    return *res;
}

template <typename T>
inline auto setOrCheck(boost::optional<T> &op)
{
    return [&op](auto &ctx) {
        if (op)
        {
            x3::_pass(ctx) = *op == x3::_attr(ctx);
        }
        else
        {
            op = T(x3::_attr(ctx));
        }
    };
}

inline auto checkLabel(OptLabel &l)
{
    auto const r = x3::rule<struct label>{"label"} =
        x3::lit("L") >> (*x3::digit)[setOrCheck(l)];
    return r;
}

inline auto checkJump(OptLabel &label)
{
    auto const r = x3::rule<struct jump>{"jump"} =
        x3::lit("BRA") > checkLabel(label);
    return r;
}

inline auto branchToEnd(OptLabel &label)
{
    auto const r = x3::rule<struct branch_to_end>{"branch_to_end"} =
        checkJump(label) > checkLabel(label) >> ":";
    return r;
}

template <typename CheckMain, typename CheckStrings = x3::eps_parser,
          typename CheckFunctions = x3::eps_parser>
inline void checkProgram(const std::string &program, const CheckMain &checkMain,
                  const CheckStrings &checkStrings = x3::eps,
                  const CheckFunctions &checkFunctions = x3::eps, bool checkBranchToEnd = true)
{
    CAPTURE(program);
    auto iter = program.begin();
    auto end = program.end();

    // Our error handler
    struct checker_class : error_handler
    {
    };
    OptLabel endLabel;
    auto branchToEndChecker = x3::rule<struct branch_to_end_checker>{} = x3::eps(!checkBranchToEnd) | branchToEnd(endLabel);

    auto const checker = x3::rule<checker_class>{"checker"} =
        checkStrings > checkFunctions > "main:" > checkMain > branchToEndChecker;

    std::stringstream sst;
    x3::error_handler<std::decay_t<decltype(iter)>> error_handler(iter, end, sst);

    auto context = x3::make_context<x3::error_handler_tag>(
        error_handler,
        x3::make_context<x3::skipper_tag>(x3::as_parser(x3::space)));

    auto parsed = checker.parse(iter, end, context, x3::unused, x3::unused);
    x3::skip_over(iter, end, context);

    if (parsed && iter != end)
    {
        error_handler(iter, "Error! Expecting end of input here: ");
    }

    if (!parsed || iter != end)
    {
        FAIL(sst.str());
    }
}

template <typename CheckDest, typename CheckSrc>
inline auto checkMove(const CheckDest &checkDest, const CheckSrc &checkSrc)
{
    auto const r = x3::rule<struct move>{"move"} =
        x3::lit("MOVE") > checkSrc >> ',' > checkDest;
    return r;
}

inline auto checkReg(OptReg &reg)
{
    auto const r = x3::rule<struct reg_>{"register"} =
        x3::lit("t") >> x3::int_[setOrCheck(reg)];
    return r;
}

inline auto checkImm(int i)
{
    auto checkImm = [](int i) {
#if 0
  return x3::int_(i);
#else
        return x3::lit(std::to_string(i));
#endif
    };

    auto const r = x3::rule<struct imm>{"imm"} = x3::lit("#") >> checkImm(i);
    return r;
}

inline std::string binOp(ir::BinOp op)
{
    if (arch == "m68k")
    {
        switch (op)
        {
        case tiger::ir::BinOp::PLUS:
            return "ADD";
        case tiger::ir::BinOp::MUL:
            return "MULS.L";
        case tiger::ir::BinOp::MINUS:
            return "SUB";
        case tiger::ir::BinOp::DIV:
            return "DIVS.L";
        default:
            break;
        }
    }

    assert(false && "Not implemented");
    return "";
}

template <typename CheckLeft, typename CheckRight>
inline auto checkBinaryOperation(ir::BinOp op, const CheckLeft &left,
                          const CheckRight &right, OptReg &result)
{
    auto const r = x3::rule<struct binop>{"binop"} =
        checkMove(checkReg(result), left) >> x3::lit(binOp(op)) > right >> ',' >
        checkReg(result);
    return r;
}

inline auto checkMemberAddress(OptReg &base, int index, OptReg &result,
                        const gsl::span<OptReg, 3> &temps)
{
    auto const r = x3::rule<struct member_address>{"member address"} =
        checkMove(checkReg(temps[0]), checkImm(index)) >
        checkMove(checkReg(temps[1]), checkImm(wordSize())) >
        checkBinaryOperation(ir::BinOp::MUL, checkReg(temps[0]),
                             checkReg(temps[1]), temps[2]) >
        checkBinaryOperation(ir::BinOp::PLUS, checkReg(base), checkReg(temps[2]),
                             result);
    return r;
}

template <typename CheckExp>
inline auto checkMemoryAccess(const CheckExp &checkExp)
{
    auto const r = x3::rule<struct memory_access>{"memory access"} =
        x3::lit('(') > checkExp >> ')';
    return r;
}

inline auto checkMemberAccess(OptReg &base, int index, OptReg &result,
                       const gsl::span<OptReg, 4> &temps)
{
    auto const r = x3::rule<struct member_access>{"member access"} =
        checkMemberAddress(base, index, temps[0], temps.subspan<1>()) >
        checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[0])));
    return r;
}

template <typename CheckExp>
inline auto checkArg(const CheckExp &checkExp)
{
    auto const r = x3::rule<struct arg>("argument") =
        checkMove(x3::lit('-') > checkMemoryAccess(x3::lit("sp")), checkExp);
    return r;
}

template <typename CheckLabel, typename CheckArgs>
inline auto checkCall(const CheckLabel &checkLabel, const CheckArgs &checkArgs)
{
    auto const r = x3::rule<struct call>{"call"} =
        checkArgs >> x3::lit("JSR") > checkLabel;
    return r;
}

template <ptrdiff_t level>
inline auto checkStaticLink(gsl::span<OptReg, level> staticLinks, gsl::span<OptReg, 2 * level> temps)
{
    auto const r = x3::rule<struct static_link>{"static link"} =
        checkStaticLink(staticLinks.template subspan<1>(), temps.template subspan<2>()) >
        checkMove(checkReg(temps[0]), checkImm(-wordSize())) >
        checkBinaryOperation(ir::BinOp::PLUS,
                             checkReg(staticLinks[1]),
                             checkReg(temps[0]), temps[1]) >
        checkMove(checkReg(staticLinks[0]), checkMemoryAccess(checkReg(temps[1])));
    return r;
}

template <>
inline auto checkStaticLink<1>(gsl::span<OptReg, 1> staticLinks, gsl::span<OptReg, 2> temps)
{
    auto const r = x3::rule<struct static_link>{"static link"} =
        checkMove(checkReg(temps[0]), checkImm(-wordSize())) >
        checkBinaryOperation(ir::BinOp::PLUS,
                             framePointer(),
                             checkReg(temps[0]), temps[1]) >
        checkMove(checkReg(staticLinks[0]), checkMemoryAccess(checkReg(temps[1])));
    return r;
}

template <size_t N>
inline auto checkStaticLink(OptReg (&staticLinks)[N], OptReg (&temps)[N * 2])
{
    return checkStaticLink(gsl::span<OptReg, N>{staticLinks}, gsl::span<OptReg, 2 * N>{temps});
}

template <typename CheckLabel, typename CheckPrepareArgs = x3::eps_parser, typename CheckArgs = x3::eps_parser>
inline auto checkLocalCall(const CheckLabel &checkLabel, OptReg &staticLink, gsl::span<OptReg, 2> temps,
                    const CheckArgs &checkArgs = x3::eps,
                    const CheckPrepareArgs &checkPrepareArgs = x3::eps)
{
    auto const r = x3::rule<struct local_call>{"local call"} =
        checkStaticLink(gsl::span<OptReg, 1>{&staticLink, 1}, temps) > checkCall(checkLabel, checkPrepareArgs > checkArg(checkReg(staticLink)) > checkArgs);
    return r;
}

template <typename CheckArgs>
inline auto checkExternalCall(const std::string &name, const CheckArgs &checkArgs)
{
    auto const r = x3::rule<struct external_call>{"external call"} =
        checkCall(x3::lit(name), checkArgs);
    return r;
}

inline auto checkString(OptLabel &stringLabel)
{
    // a string is translated to the label of the string address
    auto const r = x3::rule<struct string_rule>{"string"} =
        "#" > checkLabel(stringLabel);
    return r;
}

inline auto checkStringInit(OptLabel &stringLabel, const std::string &str)
{
    auto const r = x3::rule<struct string_init>{"string initialization"} =
        (checkLabel(stringLabel) >> ':' >> "ds.b") > x3::lexeme[str];
    return r;
}

inline std::string conditionalJump(ir::RelOp op)
{
    if (arch == "m68k")
    {
        std::stringstream sst;
        sst << op;
        return "B" + sst.str();
    }

    assert(false && "Not implemented");
    return "";
}

inline auto checkConditionalJump(ir::RelOp op, OptReg &left, OptReg &right, OptLabel &destination)
{
    auto const r = x3::rule<struct conditional_jump>{"conditional jump"} =
        x3::lit(binOp(ir::BinOp::MINUS)) > checkReg(left) > ',' >
        checkReg(right) > x3::lit(conditionalJump(op)) > checkLabel(destination);
    return r;
}

inline auto checkInlineParameterAccess(int index, OptReg &reg)
{
    auto const r = x3::rule<struct parameter_access>{"parameter access"} =
        (x3::eps(arch == "m68k") > checkMemoryAccess(checkImm(-wordSize() * (index + 2)) > ',' > framePointer())) |
        (checkReg(reg));
    return r;
}

template <typename CheckBase>
inline auto checkParameterAcess(int index, const CheckBase &checkBase, gsl::span<OptReg, 2> temps, OptReg &result)
{
    auto const r = x3::rule<struct parameter_access>{"parameter access"} =
        (x3::eps(arch == "m68k") >
         checkMove(checkReg(temps[0]), checkImm(-wordSize() * 2 * (index + 1))) >
         checkBinaryOperation(ir::BinOp::PLUS, checkBase, checkReg(temps[0]), temps[1]) >
         checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[1])))) |
        (checkReg(result));
    return r;
}