#include "TempMap.h"
#include "Tree.h"
#include "warning_suppress.h"
#include <array>
#include <boost/optional/optional_io.hpp>
MSC_DIAG_OFF(4496 4459 4127 4819)
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
MSC_DIAG_ON()
#include <boost/format.hpp>
#include <catch2/catch.hpp>
#include <gsl/span>

extern std::string arch;

namespace x3   = boost::spirit::x3;
namespace ir   = tiger::ir;
namespace temp = tiger::temp;
using OptReg   = boost::optional<temp::Register>;
using OptLabel = boost::optional<temp::Label>;

struct ErrorHandler {
  template <typename Iterator, typename Exception, typename Context>
  x3::error_handler_result
    on_error(Iterator & /* first */, Iterator const & /* last */,
             Exception const &x, Context const &context) {
    auto &error_handler = x3::get<x3::error_handler_tag>(context);
    std::string message = "Error! Expecting: " + x.which() + " here:";
    error_handler(x.where(), message);
    return x3::error_handler_result::fail;
  }
};

inline auto returnReg() {
  auto const r = x3::rule<struct return_reg>{"return reg"} =
    (arch == "m68k") ? x3::lit("D0") : x3::lit("RAX");
  return r;
}

inline auto wordSize() {
  if (arch == "m68k") {
    return 4;
  }

  return 8;
}

inline auto framePointer() {
  auto const r = x3::rule<struct frame_pointer>{"frame pointer"} =
    (arch == "m68k") ? x3::lit("A6") : x3::lit("RBP");
  return r;
}

std::string checkedCompile(const std::string &string);

template <typename T> inline auto setOrCheck(boost::optional<T> &op) {
  return [&op](auto &ctx) {
    if (op) {
      x3::_pass(ctx) = *op == x3::_attr(ctx);
    } else {
      op = T(x3::_attr(ctx));
    }
  };
}

inline auto checkLabel(OptLabel &l) {
  auto const r = x3::rule<struct label>{"label"} =
    x3::lit("L") >> (*x3::digit)[setOrCheck(l)];
  return r;
}

inline auto checkJump(OptLabel &label) {
  auto const r = x3::rule<struct jump>{"jump"} =
    ((arch == "m68k") ? x3::lit("BRA") : x3::lit("jmp")) > checkLabel(label);
  return r;
}

inline auto branchToEnd(OptLabel &label) {
  auto const r = x3::rule<struct branch_to_end>{"branch_to_end"} =
    checkJump(label) > checkLabel(label) >> ":";
  return r;
}

template <typename... Expressions>
inline void checkProgram(const std::string &program,
                         const Expressions &... expressions) {
  CAPTURE(program);
  auto iter = program.begin();
  auto end  = program.end();

  std::stringstream sst;
  using Iterator = std::decay_t<decltype(iter)>;
  x3::error_handler<Iterator> error_handler(iter, end, sst);

  auto parseExpression = [&](Iterator &start, Iterator &end,
                             const auto &expression) {
    auto r = x3::rule<ErrorHandler>{"with_error_handler"} = expression;
    auto const parser =
      x3::with<x3::error_handler_tag>(std::ref(error_handler))[r];
    auto parsed = x3::phrase_parse(start, end, parser, x3::space);
    if (!parsed) {
      FAIL(sst.str());
    }
  };

  // parse all expressions
  (void)std::initializer_list<int>{
    (parseExpression(iter, end, expressions), 0)...};

  if (iter != end) {
    error_handler(iter, "Error! Expecting end of input here: ");
    FAIL(sst.str());
  }
}

inline auto checkMain() {
  auto r = x3::rule<struct main>{"main"} = x3::lit("main:");
  return r;
}

template <typename CheckDest, typename CheckSrc>
inline auto checkMove(const CheckDest &checkDest, const CheckSrc &checkSrc) {
  auto const r = x3::rule<struct move>{"move"} =
    (x3::eps(arch == "m68k") > x3::lit("MOVE") > checkSrc >> ',' > checkDest)
    | (x3::eps(arch == "x64") > x3::lit("mov") > checkDest > ',' > checkSrc);
  return r;
}

inline auto checkReg(OptReg &reg) {
  auto const r = x3::rule<struct reg_>{"register"} =
    x3::lit("t") >> x3::int_[setOrCheck(reg)];
  return r;
}

inline auto checkImm(int i) {
  auto checkImm = [](int i) {
#if 0
  return x3::int_(i);
#else
    return x3::lit(std::to_string(i));
#endif
  };

  auto const r = x3::rule<struct imm>{"imm"} =
    (x3::eps(arch == "x64") | x3::lit("#")) >> checkImm(i);
  return r;
}

inline std::string binOp(ir::BinOp op) {
  if (arch == "m68k") {
    switch (op) {
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
  } else {
    switch (op) {
      case tiger::ir::BinOp::PLUS:
        return "add";
      case tiger::ir::BinOp::MUL:
        return "imul";
      case tiger::ir::BinOp::MINUS:
        return "sub";
      case tiger::ir::BinOp::DIV:
        return "idiv";
      default:
        break;
    }
  }

  assert(false && "Not implemented");
  return "";
}

template <typename CheckLeft, typename CheckRight>
inline auto checkBinaryOperation(ir::BinOp op, const CheckLeft &left,
                                 const CheckRight &right, OptReg &result) {
  auto const r = x3::rule<struct binop>{"binop"} =
    (x3::eps(arch == "x64" && op == ir::BinOp::DIV)
     > checkMove(x3::lit("EAX"), left) > x3::lit(binOp(op)) > right
     > checkMove(checkReg(result), x3::lit("EAX")))
    | (checkMove(checkReg(result), left) >> x3::lit(binOp(op))
       > ((x3::eps(arch == "m68k") > right > ',' > checkReg(result))
          | (x3::eps(arch == "x64") > checkReg(result) > ',' > right)));
  return r;
}

inline auto checkMemberAddress(OptReg &base, int index, OptReg &result,
                               const gsl::span<OptReg, 3> &temps) {
  auto const r = x3::rule<struct member_address>{"member address"} =
    checkMove(checkReg(temps[0]), checkImm(index))
    > checkMove(checkReg(temps[1]), checkImm(wordSize()))
    > checkBinaryOperation(ir::BinOp::MUL, checkReg(temps[0]),
                           checkReg(temps[1]), temps[2])
    > checkBinaryOperation(ir::BinOp::PLUS, checkReg(base), checkReg(temps[2]),
                           result);
  return r;
}

template <typename CheckExp>
inline auto checkMemoryAccess(const CheckExp &checkExp) {
  auto const r = x3::rule<struct memory_access>{"memory access"} =
    (arch == "m68k" ? x3::lit('(') : x3::lit('[')) > checkExp
    >> (arch == "m68k" ? ')' : ']');
  return r;
}

inline auto checkMemberAccess(OptReg &base, int index, OptReg &result,
                              const gsl::span<OptReg, 4> &temps) {
  auto const r = x3::rule<struct member_access>{"member access"} =
    checkMemberAddress(base, index, temps[0], temps.subspan<1>())
    > checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[0])));
  return r;
}

template <typename CheckExp>
inline auto checkArg(int index, const CheckExp &checkExp) {
  auto argReg = [](int index) -> std::string {
    if (arch == "x64") {
      assert(index < 4 && "index out of bounds");
      std::string argRegs[] = {"RCX", "RDX", "R8", "R9"};
      return argRegs[index];
    }

    return {};
  };
  auto const r = x3::rule<struct arg>("argument") =
    (x3::eps(arch == "m68k")
     > checkMove(x3::lit('-') > checkMemoryAccess(x3::lit("sp")), checkExp))
    | (x3::eps(arch == "x64")
       > ((x3::eps(index < 4) > checkMove(x3::lit(argReg(index)), checkExp))
          | (x3::eps(index >= 4) > "push" > checkExp)));
  return r;
}

template <typename CheckLabel, typename CheckArgs>
inline auto checkCall(const CheckLabel &checkLabel,
                      const CheckArgs &checkArgs) {
  auto const r = x3::rule<struct call>{"call"} =
    checkArgs >> ((arch == "m68k") ? x3::lit("JSR") : x3::lit("call"))
    > checkLabel;
  return r;
}

template <ptrdiff_t level>
inline auto checkStaticLink(gsl::span<OptReg, level> staticLinks,
                            gsl::span<OptReg, 2 * level> temps) {
  auto const r = x3::rule<struct static_link>{"static link"} =
    checkStaticLink(staticLinks.template subspan<1>(),
                    temps.template subspan<2>())
    > checkMove(checkReg(temps[0]), checkImm(-wordSize()))
    > checkBinaryOperation(ir::BinOp::PLUS, checkReg(staticLinks[1]),
                           checkReg(temps[0]), temps[1])
    > checkMove(checkReg(staticLinks[0]),
                checkMemoryAccess(checkReg(temps[1])));
  return r;
}

template <>
inline auto checkStaticLink<1>(gsl::span<OptReg, 1> staticLinks,
                               gsl::span<OptReg, 2> temps) {
  auto const r = x3::rule<struct static_link>{"static link"} =
    checkMove(checkReg(temps[0]), checkImm(-wordSize()))
    > checkBinaryOperation(ir::BinOp::PLUS, framePointer(), checkReg(temps[0]),
                           temps[1])
    > checkMove(checkReg(staticLinks[0]),
                checkMemoryAccess(checkReg(temps[1])));
  return r;
}

template <size_t N>
inline auto checkStaticLink(OptReg (&staticLinks)[N], OptReg (&temps)[N * 2]) {
  return checkStaticLink(gsl::span<OptReg, N>{staticLinks},
                         gsl::span<OptReg, 2 * N>{temps});
}

inline auto checkStaticLink(OptReg &staticLink, OptReg (&temps)[2]) {
  return checkStaticLink(gsl::span<OptReg, 1>{&staticLink, 1},
                         gsl::span<OptReg, 2>{temps});
}

inline auto checkString(OptLabel &stringLabel) {
  // a string is translated to the label of the string address
  auto const r = x3::rule<struct string_rule>{"string"} =
    ((arch == "m68k") ? "#" : "") > checkLabel(stringLabel);
  return r;
}

inline auto checkStringInit(OptLabel &stringLabel, const std::string &str) {
  auto const r = x3::rule<struct string_init>{"string initialization"} =
    (checkLabel(stringLabel) >> ':' >> ".string") > x3::lexeme[str];
  return r;
}

inline std::string conditionalJump(ir::RelOp op) {
  if (arch == "m68k") {
    std::stringstream sst;
    sst << op;
    return "B" + sst.str();
  } else {
    switch (op) {
      case ir::RelOp::EQ:
        return "je";
      case ir::RelOp::NE:
        return "jne";
      case ir::RelOp::LT:
        return "jl";
      case ir::RelOp::GT:
        return "jg";
      case ir::RelOp::LE:
        return "jle";
      case ir::RelOp::GE:
        return "jge";
      default:
        break;
    }
  }

  assert(false && "Not implemented");
  return "";
}

inline auto checkConditionalJump(ir::RelOp op, OptReg &left, OptReg &right,
                                 OptLabel &destination) {
  auto const r = x3::rule<struct conditional_jump>{"conditional jump"} =
    x3::lit(binOp(ir::BinOp::MINUS))
    > ((x3::eps(arch == "m68k") > checkReg(left) > ',' > checkReg(right))
       | (x3::eps(arch == "x64") > checkReg(right) > ',' > checkReg(left)))
    > x3::lit(conditionalJump(op)) > checkLabel(destination);
  return r;
}

inline auto checkInlineParameterAccess(int index, OptReg &reg) {
  auto const r = x3::rule<struct parameter_access>{"parameter access"} =
    (x3::eps(arch == "m68k")
     > checkMemoryAccess(checkImm(-wordSize() * (index + 2)) > ','
                         > framePointer()))
    | (checkReg(reg));
  return r;
}

template <typename CheckBase>
inline auto checkParameterAcess(int index, const CheckBase &checkBase,
                                gsl::span<OptReg, 2> temps, OptReg &result,
                                bool escapes = false) {
  auto const r = x3::rule<struct parameter_access>{"parameter access"} =
    (x3::eps(arch == "m68k" || escapes)
     > checkMove(checkReg(temps[0]), checkImm(-wordSize() * 2 * (index + 1)))
     > checkBinaryOperation(ir::BinOp::PLUS, checkBase, checkReg(temps[0]),
                            temps[1])
     > checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[1]))))
    | x3::eps(arch == "x64");
  return r;
}
