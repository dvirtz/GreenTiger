#include "Program.h"
#include "Tree.h"
#include "irange.h"
#include "warning_suppress.h"
#include <array>
#include <boost/optional/optional_io.hpp>
#include <set>
MSC_DIAG_OFF(4496 4459 4127 4819)
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>
MSC_DIAG_ON()
#include <boost/format.hpp>
#include <boost/fusion/include/all.hpp>
#include <boost/fusion/include/fold.hpp>
#include <boost/fusion/include/std_tuple.hpp>
#define CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER
#include <catch2/catch.hpp>
#include <gsl/span>
MSC_DIAG_OFF(4702 4172)
#include <range/v3/action/drop.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/action/take.hpp>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/empty.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp>
#include <range/v3/view/single.hpp>
#include <range/v3/view/slice.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>
MSC_DIAG_ON()

namespace x3   = boost::spirit::x3;
namespace ir   = tiger::ir;
namespace temp = tiger::temp;

using OptReg   = boost::optional<temp::Register>;
using OptLabel = boost::optional<temp::Label>;
using OptIndex = boost::optional<int>;

template <typename T> struct is_reg : std::false_type {};
template <> struct is_reg<temp::Register> : std::true_type {};
template <> struct is_reg<OptReg> : std::true_type {};

extern std::string arch;

namespace tiger {
class Machine;
namespace frame {
class CallingConvention;
}
} // namespace tiger

class TestFixture {
protected:
  template <typename ID, typename T> struct pair {
    using first_type  = ID;
    using second_type = T;
  };
  using iterator = std::string::const_iterator;
  using parser   = x3::any_parser<
    iterator, x3::unused_type,
    x3::subcontext<pair<x3::skipper_tag, decltype(x3::space)>,
                   pair<x3::error_handler_tag, x3::error_handler<iterator>>>>;

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

  class escapes {
  public:
    explicit escapes(OptReg &optReg) : m_optReg{optReg} {}

    operator OptReg &() { return m_optReg; }

  private:
    OptReg &m_optReg;
  };

  tiger::Machine const &machine() const;

  tiger::frame::CallingConvention const &callingConvention() const;

  temp::Register returnReg() const;

  int wordSize() const;

  temp::Register framePointer() const;

  temp::Register stackPointer() const;

  const temp::Registers &liveAtExitRegisters() const;

  const temp::PredefinedRegisters &predefinedRegisters() const;

  const temp::Registers &argumentRegisters() const;

  const temp::Registers &callDefinedRegisters() const;

  template <typename T> auto setOrCheck(boost::optional<T> &op) const;

  parser checkLabel(OptLabel &l) const;

  parser checkLabel(OptLabel &l, OptIndex &i);

  parser checkReg(const temp::Register &reg);

  parser checkReg(OptReg &reg);

  tiger::CompileResults checkedCompile(const std::string &string) const;

  template <typename Rng, typename ElementChecker>
  parser checkRange(Rng &&rng, ElementChecker &&elementChecker) const;

  using IntList = std::initializer_list<int>;

  using Reg = boost::variant<ranges::reference_wrapper<OptReg>, temp::Register>;
  struct RegList : std::vector<Reg> {
    using base = std::vector<Reg>;
    using base::base;

    template <typename Cont,
              typename = std::enable_if_t<std::is_constructible<
                base, decltype(ranges::begin(std::declval<Cont&>())),
                decltype(ranges::end(std::declval<Cont&&>()))>::value>>
    RegList(Cont &&registers) :
        base{ranges::begin(std::forward<Cont>(registers)), ranges::end(std::forward<Cont>(registers))} {}
  };

  template <typename SuccessorsChecker>
  parser checkAttributes(SuccessorsChecker successorChecker,
                         const RegList &uses = {}, const RegList &defs = {},
                         const RegList &liveRegisters = {},
                         bool isMove                  = false);

  parser checkInt(int i);

  parser checkImm(int i);

  parser checkFallthrough();

  parser checkJumpToLabels(
    std::initializer_list<std::reference_wrapper<OptIndex>> labelIndices);

  parser checkNoSuccessors();

  parser checkJump(OptLabel &label, OptIndex &labelIndex);

  template <typename... Parsers> auto combineParsers(Parsers &&... parsers);

  parser branchToEnd(OptLabel &label, OptIndex &labelIndex);

  template <typename... Expressions>
  void checkProgram(const tiger::CompileResults &program,
                    Expressions &&... expressions);

  parser checkMain();

  template <typename CheckDest, typename CheckSrc,
            typename = std::enable_if_t<!is_reg<CheckDest>::value>>
  parser checkMove(const CheckDest &checkDest, const CheckSrc &checkSrc,
                   const RegList &uses, const RegList &defs = {},
                   bool isMove = false, const RegList &liveRegisters = {});

  template <typename T,
            typename = std::enable_if_t<is_reg<std::decay_t<T>>::value>>
  parser checkMove(T &&dest, int src, const RegList &liveRegisters = {});

  template <typename Dst, typename Src,
            typename = std::enable_if_t<is_reg<std::decay_t<Dst>>::value
                                        && is_reg<std::decay_t<Src>>::value>>
  parser checkMove(Dst &&dest, Src &&src, const RegList &liveRegisters = {});

  std::string binOp(ir::BinOp op) const;

  template <typename LHS, typename RHS>
  parser checkBinaryOperation(ir::BinOp op, LHS &&left, RHS &&right,
                              OptReg &result,
                              const RegList &liveRegisters = {});

  parser checkMemberAddress(OptReg &base, int memberIndex, OptReg &result,
                            const gsl::span<OptReg, 3> &temps,
                            const RegList &liveRegisters = {});

  template <typename CheckExp>
  parser checkMemoryAccess(const CheckExp &checkExp);

  parser checkMemberAccess(OptReg &base, int memberIndex, OptReg &result,
                           const gsl::span<OptReg, 4> &temps,
                           const RegList &liveRegisters = {});

  parser checkString(OptLabel &stringLabel);

  template <typename CheckTarget, typename... Args>
  parser checkCall(CheckTarget &&checkTarget, const RegList &liveRegisters,
                   Args &&... args);

  template <ptrdiff_t N>
  parser checkStaticLink(OptReg &staticLink, gsl::span<OptReg, N> temps,
                         const RegList &liveRegisters = {});

  parser checkStaticLink(OptReg &staticLink, gsl::span<OptReg, 2> temps,
                         const RegList &liveRegisters = {});

  template <size_t N>
  parser checkStaticLink(OptReg &staticLink, OptReg (&temps)[N],
                         const RegList &liveRegisters = {});

  parser checkStaticLink(OptReg &staticLink, OptReg (&temps)[2],
                         const RegList &liveRegisters = {});

  parser checkStringInit(OptLabel &stringLabel, const std::string &str,
                         OptIndex &labelIndex);

  std::string conditionalJump(ir::RelOp op) const;

  parser checkConditionalJump(ir::RelOp op, OptReg &left, OptReg &right,
                              OptLabel &destination, OptIndex &trueLabelIndex,
                              OptIndex &falseLabelIndex,
                              const RegList &liveRegisters = {});

  template <typename CheckSrc>
  parser checkInlineParameterWrite(size_t index, OptReg &reg,
                                   CheckSrc &&checkSrc, RegList uses = {},
                                   RegList defs                 = {},
                                   const RegList &liveRegisters = {},
                                   bool ecapes                  = false);

  template <typename Dest>
  parser checkInlineParameterRead(size_t index, OptReg &reg, Dest &&dest,
                                  const RegList &liveRegisters = {},
                                  bool escapes                 = false);

  template <typename BaseReg>
  parser checkParameterAccess(int parameterIndex, BaseReg &&baseReg,
                              gsl::span<OptReg, 2> temps, OptReg &result,
                              const RegList &liveRegisters = {},
                              bool escapes                 = false);

  template <typename... ArgRegs>
  parser checkFunctionStart(OptLabel &label, OptIndex &labelIndex,
                            gsl::span<OptReg, 5 * sizeof...(ArgRegs)> temps,
                            ArgRegs &&... argRegs);

  parser checkFunctionExit();

  using InterferenceGraph  = tiger::CompileResults::InterferenceGraph;
  using InterferenceGraphs = std::vector<InterferenceGraph>;

  void checkInterference(const InterferenceGraphs &interefernceGraphs) const;

  std::string regToString(const temp::Register &reg) const;

  static temp::Register actual(const Reg &reg);

  parser addDef(const Reg &reg);

  parser resetReg(OptReg &r) const;

private:
  struct regs_parser : x3::parser<regs_parser> {
    regs_parser(TestFixture &fixture, const RegList &regs) :
        m_fixture{fixture}, m_regs{regs} {}

    using attribute_type = x3::unused_type;

    template <typename Iterator, typename Context>
    bool parse(Iterator &first, Iterator const &last, Context const &context,
               x3::unused_type, x3::unused_type) const;

    TestFixture &m_fixture;
    RegList m_regs;
  };

  template <typename T> struct does_escape : std::false_type {};

  template <typename T> struct capture { T value; };

  parser startFragment();

  parser checkInlineParameterAccess(size_t index, OptReg &reg,
                                    const RegList &liveRegisters, bool escapes);

  parser addInterference(const Reg &r, const Reg &s);

  parser addInterferences(const RegList &rs, const RegList &ss);

  void addInterferenceImpl(const temp::Register &r, const temp::Register &s);

  template <typename... Args, size_t... I>
  parser checkArgs(const RegList &liveRegisters, std::index_sequence<I...>,
                   Args &&... args);

  template <typename Arg>
  parser checkArg(size_t index, Arg &&arg, const RegList &liveRegisters);

  template <typename Rng, typename ElementChecker>
  parser checkRangeImpl(Rng &&rng, ElementChecker &&elementChecker) const;

  template <typename Base>
  parser checkStaticLinkImpl(OptReg &staticLink, OptReg &temp0, OptReg &temp1,
                             Base &&base, const RegList &liveRegisters);

  auto liveRegistersAnd(const RegList &liveRegisters) const;

  template <typename Arg>
  parser checkParameterMove(size_t index, size_t argumentCount, Arg &&arg,
                            gsl::span<OptReg, 5> temps);

  template <typename... ArgRegs, size_t... I>
  parser checkParameterMoves(std::index_sequence<I...>,
                             gsl::span<OptReg, 5 * sizeof...(ArgRegs)> temps,
                             ArgRegs &&... argRegs);

  template <typename BaseReg>
  parser checkStackAccess(int offset, BaseReg &&baseReg, OptReg &temp,
                          OptReg &result, const RegList &liveRegisters = {});

  int m_instIndex = 0;
  InterferenceGraphs m_interferences;
  OptReg m_mainStaticLink;
  OptReg m_mainTemps[5];
};

template <> struct TestFixture::does_escape<TestFixture::escapes> : std::true_type {};

template <typename T>
inline auto TestFixture::setOrCheck(boost::optional<T> &op) const {
  return [&op](auto &ctx) {
    if (op) {
      x3::_pass(ctx) = *op == static_cast<T>(x3::_attr(ctx));
    } else {
      op = static_cast<T>(x3::_attr(ctx));
    }
  };
}

inline TestFixture::parser TestFixture::addInterference(const Reg &r, const Reg &s) {
  auto const toOpt = [](const Reg &r) {
    return helpers::match(r)(
      [](const temp::Register &r) -> OptReg { return r; },
      [](const OptReg &r) { return r; });
  };
  auto const addInterference = [=](auto &) {
    auto const optR = toOpt(r);
    auto const optS = toOpt(s);
    if (optR && optS) {
      addInterferenceImpl(*optR, *optS);
    }
  };

  return x3::eps[addInterference];
}

inline TestFixture::parser TestFixture::addInterferences(const RegList &rs,
                                            const RegList &ss) {
  return checkRangeImpl(rs, [this, &ss](const Reg &r) {
    return checkRangeImpl(
      ss, [this, r](const Reg &s) { return addInterference(r, s); });
  });
}

inline TestFixture::parser TestFixture::addDef(const Reg &reg) {
  return addInterferences(liveAtExitRegisters(), {reg});
}

inline TestFixture::parser TestFixture::checkReg(const temp::Register &reg) {
  static auto const predefinedRegisters = this->predefinedRegisters();
  auto it                               = predefinedRegisters.find(reg);
  assert(it != predefinedRegisters.end());
  auto const r = x3::rule<struct reg_>{it->second.c_str()} =
    x3::lit(it->second);
  return r;
}

inline TestFixture::parser TestFixture::checkReg(OptReg &reg) {
  static auto const predefinedRegisters = this->predefinedRegisters();
  static const x3::symbols<temp::Register> regsParser{
    predefinedRegisters | ranges::views::values,
    predefinedRegisters | ranges::views::keys, "regsParser"};
  auto const r = x3::rule<struct reg_>{"register"} =
    ((x3::lit("t") >> x3::int_[setOrCheck(reg)]) | regsParser[setOrCheck(reg)]);
  return r;
}

template <typename Rng, typename ElementChecker>
inline TestFixture::parser
  TestFixture::checkRangeImpl(Rng &&rng,
                              ElementChecker &&elementChecker) const {
  return ranges::accumulate(
    std::forward<Rng>(rng), parser{x3::eps},
    [checkElement = capture<ElementChecker>{std::forward<ElementChecker>(
       elementChecker)}](const parser &current, const auto &element) {
      return parser{current > checkElement.value(element)};
    });
}

template <typename Rng, typename ElementChecker>
inline TestFixture::parser TestFixture::checkRange(Rng &&rng,
                                      ElementChecker &&elementChecker) const {
  return "{" > checkRangeImpl(
                 std::forward<Rng>(rng),
                 [checkElement =
                    capture<ElementChecker>{std::forward<ElementChecker>(
                      elementChecker)}](const auto &element) {
                   return checkElement.value(element) > -x3::lit(',');
                 })
         > "}";
}

template <typename SuccessorsChecker>
inline TestFixture::parser TestFixture::checkAttributes(
  SuccessorsChecker successorChecker, const RegList &uses /*= {}*/,
  const RegList &defs /*= {}*/, const RegList &liveRegisters /*= {}*/,
  bool isMove /*= false*/) {
  auto const incrementInstIndex = [this](auto &) { ++m_instIndex; };
  auto const addDefs =
    checkRangeImpl(defs, [this](const Reg &r) { return addDef(r); });
  auto const r = x3::rule<struct attributes>{"attributes"} =
    x3::lit(';') > x3::lit("successors:") > successorChecker
    > x3::eps[incrementInstIndex] > x3::lit("uses:") > regs_parser{*this, uses}
    > x3::lit("defs:") > regs_parser{*this, defs}
    > addInterferences(liveRegisters, defs) > addDefs > x3::lit("isMove:")
    > ((x3::eps(isMove) > x3::true_) | x3::eps > x3::false_);
  return r;
}

template <typename... Parsers>
inline auto TestFixture::combineParsers(Parsers &&... parsers) {
  return boost::fusion::fold(std::make_tuple(std::forward<Parsers>(parsers)...),
                             x3::eps, std::greater<>{});
}

template <typename... Expressions>
inline void TestFixture::checkProgram(const tiger::CompileResults &results,
                                      Expressions &&... expressions) {
  CAPTURE(arch);
  CAPTURE(results.m_assembly);
  CAPTURE(results.m_interferenceGraphs);

  auto const &program = results.m_assembly;

  auto iter = program.begin();
  auto end  = program.end();

  std::stringstream sst;
  using Iterator = std::decay_t<decltype(iter)>;

  auto const combined = x3::rule<ErrorHandler>{"with_error_handler"} =
    combineParsers(std::forward<Expressions>(expressions)...);

  x3::error_handler<Iterator> error_handler(iter, end, sst);
  auto const parser =
    x3::with<x3::error_handler_tag>(std::ref(error_handler))[combined];
  auto parsed = x3::phrase_parse(iter, end, parser, x3::space);
  if (!parsed) {
    FAIL(sst.str());
  }

  if (iter != end) {
    error_handler(iter, "Error! Expecting end of input here: ");
    FAIL(sst.str());
  }

  checkInterference(results.m_interferenceGraphs);
}

inline TestFixture::parser TestFixture::checkFallthrough() {
  auto const checkIndex = [this](auto &ctx) {
    x3::_pass(ctx) = m_instIndex + 1 == x3::_attr(ctx);
  };
  auto const r = x3::rule<struct fallthrogh>{"fallthrough"} =
    x3::lit('{') > x3::int_[checkIndex] > '}';
  return r;
}

inline TestFixture::parser TestFixture::checkInt(int i) {
#if 0
  return x3::int_(i);
#else
  return x3::lit(std::to_string(i));
#endif
}

inline TestFixture::parser TestFixture::checkImm(int i) {
  auto const r = x3::rule<struct imm>{"imm"} =
    (x3::eps(arch == "x64") | x3::lit("#")) > checkInt(i);
  return r;
}

template <typename CheckDest, typename CheckSrc, typename>
inline TestFixture::parser
  TestFixture::checkMove(const CheckDest &checkDest, const CheckSrc &checkSrc,
                         const RegList &uses, const RegList &defs /*= {}*/,
                         bool isMove /*= false*/,
                         const RegList &liveRegisters /*= {}*/) {
  auto const r = x3::rule<struct move>{"move"} =
    ((x3::eps(arch == "m68k") > x3::lit("MOVE") > checkSrc > ',' > checkDest)
     | (x3::eps(arch == "x64") > x3::lit("mov") > checkDest > ',' > checkSrc))
    > checkAttributes(checkFallthrough(), uses, defs, liveRegisters, isMove);
  return r;
}

template <typename T, typename>
inline TestFixture::parser TestFixture::checkMove(T &&dest, int src,
                                     const RegList &liveRegisters /*= {}*/) {
  return checkMove(checkReg(std::forward<T>(dest)), checkImm(src), {}, {dest},
                   false, liveRegisters);
}

template <typename Dst, typename Src, typename>
inline TestFixture::parser TestFixture::checkMove(Dst &&dest, Src &&src,
                                     const RegList &liveRegisters /*= {}*/) {
  return checkMove(checkReg(dest), checkReg(src), {src}, {dest}, true,
                   liveRegisters);
}

inline auto TestFixture::liveRegistersAnd(const RegList &liveRegisters) const {
  return [&liveRegisters](auto&&... regs) {
    return ranges::views::concat(liveRegisters, ranges::views::single(Reg{regs})...);
  };
}

template <typename LHS, typename RHS>
inline TestFixture::parser
  TestFixture::checkBinaryOperation(ir::BinOp op, LHS &&left, RHS &&right,
                                    OptReg &result,
                                    const RegList &liveRegisters /*= {}*/) {
  auto const liveRegistersAnd = this->liveRegistersAnd(liveRegisters);
  auto const x64Div =
    x3::eps(arch == "x64" && op == ir::BinOp::DIV)
    > checkMove(returnReg(), std::forward<LHS>(left),
                liveRegistersAnd(std::forward<RHS>(right)))
    > x3::lit(binOp(op)) > checkReg(std::forward<RHS>(right))
    > checkAttributes(checkFallthrough(), {right, returnReg()}, {returnReg()},
                      liveRegistersAnd(returnReg()))
    > checkMove(result, returnReg(), liveRegisters);
  auto const r = x3::rule<struct binop>{"binop"} =
    x64Div
    | (checkMove(result, std::forward<LHS>(left),
                 liveRegistersAnd(std::forward<RHS>(right)))
       > x3::lit(binOp(op))
       > ((x3::eps(arch == "m68k") > checkReg(std::forward<RHS>(right)) > ','
           > checkReg(result))
          | (x3::eps(arch == "x64") > checkReg(result) > ','
             > checkReg(std::forward<RHS>(right))))
       > checkAttributes(checkFallthrough(), {right, result}, {result},
                         liveRegisters));
  return r;
}

template <typename CheckExp>
inline TestFixture::parser TestFixture::checkMemoryAccess(const CheckExp &checkExp) {
  auto const r = x3::rule<struct memory_access>{"memory access"} =
    (arch == "m68k" ? x3::lit('(') : x3::lit('[')) > checkExp
    >> (arch == "m68k" ? ')' : ']');
  return r;
}

inline TestFixture::parser TestFixture::checkLabel(OptLabel &l) const {
  auto const r =
    x3::rule<struct label_reference, std::string>{"label reference"} =
      x3::lexeme[x3::alpha >> *x3::alnum];
  return r[setOrCheck(l)];
}

inline TestFixture::parser TestFixture::checkLabel(OptLabel &l, OptIndex &i) {
  auto const r = x3::rule<struct label>{"label"} =
    checkLabel(l) > x3::attr(ranges::ref(m_instIndex))[setOrCheck(i)] > ':'
    > checkAttributes(checkFallthrough());
  return r;
}

inline TestFixture::parser TestFixture::checkString(OptLabel &stringLabel) {
  // a string is translated to the label of the string address
  auto const r = x3::rule<struct string_rule>{"string"} =
    ((arch == "m68k") ? "#" : "") > checkLabel(stringLabel);
  return r;
}

template <typename Arg>
inline TestFixture::parser TestFixture::checkArg(size_t index, Arg &&arg,
                                    const RegList &liveRegisters) {
  auto argChecker = helpers::overload(
    [this](int i) { return checkImm(i); },
    [this](OptReg &reg) { return checkReg(reg); },
    [this](OptLabel &label) { return checkString(label); })(arg);

  auto uses =
    helpers::overload([](OptReg &reg) { return RegList{reg}; },
                      [](auto && /*default*/) { return RegList{}; })(arg);

  auto const r = x3::rule<struct arg_>("argument") = [&]() -> parser {
    if (arch == "m68k") {
      return checkMove(
        x3::lit('+') > checkMemoryAccess(checkReg(stackPointer())), argChecker,
        ranges::views::concat(ranges::views::single(stackPointer()), uses));
    }

    static const auto &argumentRegisters = this->argumentRegisters();
    if (index < argumentRegisters.size()) {
      return checkMove(checkReg(argumentRegisters[index]), argChecker, uses,
                       {argumentRegisters[index]}, uses.size() == 1,
                       liveRegisters);
    }

    uses.emplace_back(stackPointer());
    return checkMove(
      checkMemoryAccess(binOp(ir::BinOp::PLUS) > checkReg(stackPointer())
                        > checkInt(static_cast<int>(index * wordSize()))),
      argChecker, uses, {}, false, liveRegisters);
  }();
  return r;
}

template <typename... Args, size_t... I>
inline TestFixture::parser TestFixture::checkArgs(const RegList &liveRegisters,
                                     std::index_sequence<I...>,
                                     Args &&... args) {
  using namespace ranges;
  auto const toOptReg = helpers::overload(
    [](OptReg &r) {
      return boost::optional<Reg>{true, r};
    },
    [](const temp::Register &r) { return boost::optional<Reg>{r}; },
    [](const auto &) { return boost::optional<Reg>{}; });
  auto const arguments   = views::concat(views::single(toOptReg(args))...);
  auto withLiveArguments = [&liveRegisters, arguments](size_t i) -> RegList {
    std::vector<boost::optional<Reg>> a{
      arguments | views::drop(i + 1)
      | views::filter(
          [](const boost::optional<Reg> &r) { return r.is_initialized(); })};
    return views::concat(
      liveRegisters,
      views::transform(a, [](const boost::optional<Reg> &r) { return *r; }));
  };
  return combineParsers(checkArg(I, args, withLiveArguments(I))...);
}

template <typename CheckTarget, typename... Args>
inline TestFixture::parser TestFixture::checkCall(CheckTarget &&checkTarget,
                                     const RegList &liveRegisters,
                                     Args &&... args) {
  auto const r = x3::rule<struct call>{"call"} =
    checkArgs(liveRegisters, std::index_sequence_for<Args...>{},
              std::forward<Args>(args)...)
    > ((arch == "m68k") ? x3::lit("JSR") : x3::lit("call")) > checkTarget
    > checkAttributes(checkFallthrough(), {}, callDefinedRegisters(),
                      liveRegisters);
  return r;
}

template <typename Base>
inline TestFixture::parser TestFixture::checkStaticLinkImpl(OptReg &staticLink,
                                               OptReg &temp0, OptReg &temp1,
                                               Base &&base,
                                               const RegList &liveRegisters) {
  auto const r = x3::rule<struct static_link>{"static link"} =
    checkMove(temp0, -wordSize(),
              liveRegistersAnd(liveRegisters)(std::forward<Base>(base)))
    > checkBinaryOperation(ir::BinOp::PLUS, std::forward<Base>(base), temp0,
                           temp1, liveRegisters)
    > checkMove(checkReg(staticLink), checkMemoryAccess(checkReg(temp1)),
                {temp1}, {staticLink}, false, liveRegisters);
  return r;
}

template <ptrdiff_t N>
inline TestFixture::parser
  TestFixture::checkStaticLink(OptReg &staticLink, gsl::span<OptReg, N> temps,
                               const RegList &liveRegisters /*= {}*/) {
  return checkStaticLink(temps[2], temps.template subspan<3>(), liveRegisters)
         > checkStaticLinkImpl(staticLink, temps[0], temps[1], temps[2],
                               liveRegisters);
}

inline TestFixture::parser
  TestFixture::checkStaticLink(OptReg &staticLink, gsl::span<OptReg, 2> temps,
                               const RegList &liveRegisters /*= {}*/) {
  return checkStaticLinkImpl(staticLink, temps[0], temps[1], framePointer(),
                             liveRegisters);
}

template <size_t N>
inline TestFixture::parser
  TestFixture::checkStaticLink(OptReg &staticLink, OptReg (&temps)[N],
                               const RegList &liveRegisters /*= {}*/) {
  return checkStaticLink(staticLink, gsl::span<OptReg, N>{temps},
                         liveRegisters);
}

inline TestFixture::parser
  TestFixture::checkStaticLink(OptReg &staticLink, OptReg (&temps)[2],
                               const RegList &liveRegisters /*= {}*/) {
  return checkStaticLink(staticLink, gsl::span<OptReg, 2>{temps},
                         liveRegisters);
}

template <typename BaseReg>
inline TestFixture::parser
  TestFixture::checkStackAccess(int offset, BaseReg &&baseReg, OptReg &temp,
                                OptReg &result,
                                const RegList &liveRegisters /*= {}*/) {
  return checkMove(
           temp, offset,
           liveRegistersAnd(liveRegisters)(std::forward<BaseReg>(baseReg)))
         > checkBinaryOperation(ir::BinOp::PLUS, baseReg, temp, result,
                                liveRegisters);
}

template <typename BaseReg>
inline TestFixture::parser
  TestFixture::checkParameterAccess(int parameterIndex, BaseReg &&baseReg,
                                    gsl::span<OptReg, 2> temps, OptReg &result,
                                    const RegList &liveRegisters /*= {}*/,
                                    bool escapes /*= false*/) {
  auto const r = x3::rule<struct parameter_access>{"parameter access"} =
    (x3::eps(arch == "m68k" || escapes)
     > checkStackAccess(-wordSize() * (parameterIndex + 1),
                        std::forward<BaseReg>(baseReg), temps[0], temps[1],
                        liveRegisters)
     > checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[1])),
                 {temps[1]}, {result}, false, liveRegisters))
    | x3::eps(arch == "x64");
  return r;
}

inline TestFixture::parser TestFixture::checkJumpToLabels(
  std::initializer_list<std::reference_wrapper<OptIndex>> labelIndices) {
  auto const checkLabel = [this](OptIndex &labelIndex) {
    auto const r = x3::rule<struct label_index>{"label index"} =
      x3::int_[setOrCheck(labelIndex)];
    return r;
  };
  auto const r = x3::rule<struct jump_to_labels>{"jump to labels"} =
    checkRange(labelIndices, checkLabel);
  return r;
}

inline TestFixture::parser TestFixture::checkNoSuccessors() {
  auto const r = x3::rule<struct no_successors>{"no successors"} =
    x3::lit("{}");
  return r;
}

inline TestFixture::parser TestFixture::checkJump(OptLabel &label, OptIndex &labelIndex) {
  auto const r = x3::rule<struct jump>{"jump"} =
    ((arch == "m68k") ? x3::lit("BRA") : x3::lit("jmp")) > checkLabel(label)
    > checkAttributes(checkJumpToLabels({labelIndex}));
  return r;
}

inline TestFixture::parser TestFixture::branchToEnd(OptLabel &label, OptIndex &labelIndex) {
  auto const r = x3::rule<struct branch_to_end>{"branch to end"} =
    checkJump(label, labelIndex) > checkLabel(label, labelIndex);
  return r;
}

inline TestFixture::parser TestFixture::startFragment() {
  auto const onStartFragment = [this](auto &) {
    m_instIndex = 0;
    m_interferences.emplace_back();
  };

  return x3::eps[onStartFragment];
}

inline TestFixture::parser
  TestFixture::checkMemberAddress(OptReg &base, int memberIndex, OptReg &result,
                                  const gsl::span<OptReg, 3> &temps,
                                  const RegList &liveRegisters /*= {}*/) {
  auto const liveRegistersAnd = this->liveRegistersAnd(liveRegisters);
  auto const r = x3::rule<struct member_address>{"member address"} =
    checkMove(temps[0], memberIndex, liveRegistersAnd(base) | ranges::to<RegList>())
    > checkMove(temps[1], wordSize(), liveRegistersAnd(base, temps[0]))
    > checkBinaryOperation(ir::BinOp::MUL, temps[0], temps[1], temps[2],
                           liveRegistersAnd(base))
    > checkBinaryOperation(ir::BinOp::PLUS, base, temps[2], result,
                           liveRegisters);
  return r;
}

inline TestFixture::parser
  TestFixture::checkMemberAccess(OptReg &base, int memberIndex, OptReg &result,
                                 const gsl::span<OptReg, 4> &temps,
                                 const RegList &liveRegisters /*= {}*/) {
  auto const r = x3::rule<struct member_access>{"member access"} =
    checkMemberAddress(base, memberIndex, temps[0], temps.subspan<1>(),
                       liveRegisters)
    > checkMove(checkReg(result), checkMemoryAccess(checkReg(temps[0])),
                {temps[0]}, {result}, false, liveRegisters);
  return r;
}

inline TestFixture::parser TestFixture::checkStringInit(OptLabel &stringLabel,
                                           const std::string &str,
                                           OptIndex &labelIndex) {
  auto const r = x3::rule<struct string_init>{"string initialization"} =
    startFragment() > checkLabel(stringLabel, labelIndex) > ".string"
    > x3::lexeme[str] > checkAttributes(checkNoSuccessors());
  return r;
}

inline TestFixture::parser TestFixture::checkConditionalJump(
  ir::RelOp op, OptReg &left, OptReg &right, OptLabel &destination,
  OptIndex &trueLabelIndex, OptIndex &falseLabelIndex,
  const RegList &liveRegisters /*= {}*/) {
  auto const r = x3::rule<struct conditional_jump>{"conditional jump"} =
    x3::lit(binOp(ir::BinOp::MINUS))
    > ((x3::eps(arch == "m68k") > checkReg(left) > ',' > checkReg(right))
       | (x3::eps(arch == "x64") > checkReg(right) > ',' > checkReg(left)))
    > checkAttributes(checkFallthrough(), {left, right}, {right}, liveRegisters)
    > x3::lit(conditionalJump(op)) > checkLabel(destination)
    > checkAttributes(checkJumpToLabels({trueLabelIndex, falseLabelIndex}));
  return r;
}

inline TestFixture::parser
  TestFixture::checkInlineParameterAccess(size_t index, OptReg &reg,
                                          const RegList &liveRegisters /*= {}*/,
                                          bool escapes) {
  return (x3::eps(arch == "m68k")
          > checkMemoryAccess(
              checkImm(static_cast<int>(-wordSize() * (index + 1))) > ','
              > checkReg(framePointer())))
         | (x3::eps(escapes)
            > checkMemoryAccess(
                checkReg(framePointer()) > '+'
                > checkInt(static_cast<int>(-wordSize() * (index + 1)))))
         | (x3::eps > checkReg(reg) > addInterferences(liveRegisters, {reg}));
}

template <typename CheckSrc>
inline TestFixture::parser TestFixture::checkInlineParameterWrite(
  size_t index, OptReg &reg, CheckSrc &&checkSrc, RegList uses /*= {}*/,
  RegList defs /*= {}*/, const RegList &liveRegisters /*= {}*/,
  bool escapes /*= {}*/) {
  if (arch == "m68k" || escapes) {
    uses.push_back(framePointer());
  } else {
    defs.push_back(reg);
  }
  auto isMove  = arch == "x64" && uses.size() == 1 && defs.size() == 1;
  auto const r = x3::rule<struct parameter_write>{"parameter write"} =
    checkMove(checkInlineParameterAccess(index, reg, liveRegisters, escapes),
              std::forward<CheckSrc>(checkSrc), uses, defs, isMove,
              liveRegisters);
  return r;
}

template <typename Dest>
TestFixture::parser
  TestFixture::checkInlineParameterRead(size_t index, OptReg &reg, Dest &&dest,
                                        const RegList &liveRegisters /*= {}*/,
                                        bool escapes /*= {}*/) {
  RegList uses{[&, this]() -> Reg {
    if (arch == "m68k" || escapes) {
      return framePointer();
    }
    return reg;
  }()};
  RegList defs{dest};
  auto isMove  = arch == "x64";
  auto const r = x3::rule<struct parameter_read>{"parameter read"} =
    checkMove(checkReg(dest),
              checkInlineParameterAccess(index, reg, liveRegisters, escapes),
              uses, defs, isMove, liveRegisters);
  return r;
}

inline TestFixture::parser TestFixture::checkFunctionExit() {
  return checkAttributes(checkNoSuccessors(), liveAtExitRegisters());
}

template <typename Arg>
inline TestFixture::parser TestFixture::checkParameterMove(size_t index,
                                              size_t argumentCount, Arg &&arg,
                                              gsl::span<OptReg, 5> temps) {
  auto const &argumentRegisters = this->argumentRegisters();
  auto const r =
    x3::rule<struct parameter_move>{"parameter move"} = [&]() -> parser {
    if (index < argumentRegisters.size()) {
      auto const liveRegisters =
        argumentRegisters | ranges::views::slice(index + 1, argumentCount) | ranges::to_vector;
      return checkInlineParameterWrite(
        index, std::forward<Arg>(arg), checkReg(argumentRegisters[index]),
        {argumentRegisters[index]}, {}, liveRegisters, does_escape<Arg>::value);
    }

    return checkStackAccess(static_cast<int>(index * wordSize()),
                            stackPointer(), temps[0], temps[1])
           > checkMove(checkReg(temps[2]),
                       checkMemoryAccess(checkReg(temps[1])), {temps[1]},
                       {temps[2]})
           > checkStackAccess(-wordSize() * (static_cast<int>(index + 1)),
                              framePointer(), temps[3], temps[4], {temps[2]})
           > checkMove(checkMemoryAccess(checkReg(temps[4])),
                       checkReg(temps[2]), {temps[4], temps[2]});
  }();
  return r;
}

template <typename... ArgRegs, size_t... I>
inline TestFixture::parser TestFixture::checkParameterMoves(
  std::index_sequence<I...>, gsl::span<OptReg, 5 * sizeof...(ArgRegs)> temps,
  ArgRegs &&... argRegs) {
  return combineParsers(checkParameterMove(I, sizeof...(argRegs),
                                           std::forward<ArgRegs>(argRegs),
                                           temps.template subspan<I * 5, 5>())...);
}

inline TestFixture::parser TestFixture::checkMain() {
  auto r = x3::rule<struct main>{"main"} =
    startFragment() > x3::lit("main:") > checkAttributes(checkFallthrough())
    > checkParameterMove(0, 1, escapes{m_mainStaticLink}, m_mainTemps);
  return r;
}

template <typename... ArgRegs>
inline TestFixture::parser TestFixture::checkFunctionStart(
  OptLabel &label, OptIndex &labelIndex,
  gsl::span<OptReg, 5 * sizeof...(ArgRegs)> temps, ArgRegs &&... argRegs) {
  auto const r = x3::rule<struct function_start>{"function start"} =
    startFragment() > checkLabel(label, labelIndex)
    > checkParameterMoves(std::index_sequence_for<ArgRegs...>(), temps,
                          std::forward<ArgRegs>(argRegs)...);
  return r;
}

inline TestFixture::parser TestFixture::resetReg(OptReg &r) const {
  auto const reset = [&r](auto &) { r.reset(); };
  return x3::eps(arch == "m68k")[reset] | x3::eps(arch == "x64");
}

template <typename Iterator, typename Context>
inline bool
  TestFixture::regs_parser::parse(Iterator &first, Iterator const &last,
                                  Context const &context, x3::unused_type,
                                  x3::unused_type) const {
  auto const comparartor = [](const Reg &lhs, const Reg &rhs) {
    return actual(lhs) < actual(rhs);
  };

  std::set<Reg, decltype(comparartor)> sorted{m_regs.begin(), m_regs.end(),
                                              comparartor};

  auto const regChecker = [this](const Reg &reg) {
    return helpers::match(reg)([this](const auto &r) -> TestFixture::parser {
      return m_fixture.checkReg(r);
    });
  };
  auto parser = m_fixture.checkRange(sorted, regChecker);

  Iterator save = first;
  if (!parser.parse(first, last, context, x3::unused, x3::unused)) {
    first = save;
    return false;
  }
  return true;
}
