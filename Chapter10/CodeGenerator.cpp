#include "CodeGenerator.h"
#include "CallingConvention.h"
#include "TreeAdapted.h"
#include "variantMatch.h"
#include "warning_suppress.h"
#include <boost/fusion/include/algorithm.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/zip_view.hpp>
#include <boost/optional.hpp>
MSC_DIAG_OFF(4996 4459 4456)
#include <boost/spirit/include/karma.hpp>
MSC_DIAG_ON()
#include <range/v3/algorithm/for_each.hpp>
MSC_DIAG_OFF(4913)
#include <range/v3/algorithm/generate_n.hpp>
#include <range/v3/algorithm/move.hpp>
MSC_DIAG_ON()
MSC_DIAG_OFF(4459)
#include <range/v3/algorithm/max.hpp>
MSC_DIAG_ON()
#include <range/v3/view/concat.hpp>
#include <range/v3/view/join.hpp>
#include <range/v3/view/transform.hpp>
#include <regex>
#include <sstream>

namespace tiger {
namespace assembly {

namespace traits {
template <typename... Ts> struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<boost::variant<Ts...>> : std::true_type {};
} // namespace traits

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention,
                             Patterns &&patterns) :
    m_callingConvention{callingConvention},
    m_patterns{std::move(patterns)} {}

Instructions CodeGenerator::translateFunction(const ir::Statements &statements,
                                              temp::Map &tempMap) const {
  Instructions instructions;
  ranges::for_each(statements, [&](const ir::Statement &statement) {
    auto matched = match(statement, tempMap);
    if (!matched) {
      std::stringstream sst;
      sst << "Could find a match for ";
      printStatement(sst, statement, tempMap);
      throw std::logic_error{sst.str()};
    }
    ranges::move(*matched, ranges::back_inserter(instructions));
  });

  return instructions;
}

struct DagMatcher {
  DagMatcher(const CodeGenerator &codeGenerator, temp::Map &tempMap) :
      m_codeGenerator{codeGenerator}, m_tempMap{tempMap} {}

  struct MatchData {
    Instructions m_instructions;
    Operands m_operands;
  };

  boost::optional<MatchData> match(const Dag &dag) const;

  template <typename... Ts>
  bool match(const boost::variant<Ts...> &code,
             const boost::variant<Ts...> &pattern, MatchData &matchData) const;

  template <typename T>
  bool match(const std::vector<T> &code, const std::vector<T> &pattern,
             MatchData &matchData) const;

  template <typename T>
  bool match(const T &code, const T &pattern, MatchData &matchData,
             std::enable_if_t<!std::is_same<T, ir::Placeholder>::value
                                && boost::fusion::traits::is_sequence<T>::value,
                              int> = 0) const;

  template <typename T>
  bool
    match(const T &code, const T &pattern, MatchData &matchData,
          std::enable_if_t<!std::is_same<T, ir::Placeholder>::value
                             && !traits::is_variant<T>::value
                             && !boost::fusion::traits::is_sequence<T>::value,
                           int> = 0) const;

  template <typename T>
  bool match(
    const T &code, const ir::Placeholder &placeholder, MatchData &matchData,
    std::enable_if_t<std::is_constructible<ir::Expression, T>::value, int> =
      0) const;

  bool match(const std::shared_ptr<temp::Label> &code,
             const std::shared_ptr<temp::Label> &pattern,
             MatchData &matchData) const;

  bool match(const ir::Call &code, const ir::Call &pattern,
             MatchData &matchData) const;

  template <typename T, typename U>
  bool match(const T &, const U &, MatchData &) const {
    return false;
  }

  const CodeGenerator &m_codeGenerator;
  temp::Map &m_tempMap;
};

boost::optional<Instructions>
  CodeGenerator::match(const ir::Statement &statement,
                       temp::Map &tempMap) const {
  DagMatcher dagMatcher{*this, tempMap};
  auto dag = helpers::match(statement)(
    [](const ir::ExpressionStatement &expStatement) -> Dag {
      return expStatement.exp;
    },
    [&statement](const auto & /*default*/) -> Dag { return statement; });
  auto matchData = dagMatcher.match(dag);
  if (matchData) {
    return matchData->m_instructions;
  }

  return {};
}

template <typename... Ts>
bool DagMatcher::match(const boost::variant<Ts...> &code,
                       const boost::variant<Ts...> &pattern,
                       MatchData &matchData) const {
  return helpers::match(code, pattern)(
    [&](const auto &codeVal, const auto &patternVal) {
      return this->match(codeVal, patternVal, matchData);
    });
}

template <typename T>
bool DagMatcher::match(
  const T &code, const T &pattern, MatchData &matchData,
  std::enable_if_t<!std::is_same<T, ir::Placeholder>::value
                     && boost::fusion::traits::is_sequence<T>::value,
                   int>) const {
  namespace bf = boost::fusion;

  return bf::all(bf::zip(code, pattern), [&](const auto &zipped) {
    return match(bf::at_c<0>(zipped), bf::at_c<1>(zipped), matchData);
  });
}

template <typename T>
bool DagMatcher::match(
  const T &code, const T &pattern, MatchData & /* matchData */,
  std::enable_if_t<!std::is_same<T, ir::Placeholder>::value
                     && !traits::is_variant<T>::value
                     && !boost::fusion::traits::is_sequence<T>::value,
                   int>) const {
  return code == pattern;
}

boost::optional<DagMatcher::MatchData> DagMatcher::match(const Dag &dag) const {
  for (const auto &pattern : m_codeGenerator.m_patterns) {
    MatchData matchData;
    if (!match(dag, pattern.m_dag, matchData)) {
      continue;
    }

    namespace rv = ranges::view;

    auto maxOperandIndex = ranges::max(
      pattern.m_initializers
      | rv::transform([](const InstructionInitializer &initializer) {
          return rv::all(initializer.m_explicitArguments);
        })
      | rv::join
      | rv::transform(
          [](const boost::variant<temp::Register, size_t> &arg) -> size_t {
            auto res = boost::get<size_t>(&arg);
            if (res) {
              return *res;
            }
            return 0;
          }));

    // generate operands whose index is larger than number of operands
    if (maxOperandIndex >= matchData.m_operands.size()) {
      ranges::generate_n(ranges::back_inserter(matchData.m_operands),
                         maxOperandIndex + 1 - matchData.m_operands.size(),
                         [this]() { return m_tempMap.newTemp(); });
    }

    auto const toOperands =
      rv::transform([&](const boost::variant<temp::Register, size_t> &arg) {
        return helpers::match(arg)(
          [](const temp::Register &reg) -> Operand { return reg; },
          [&matchData](size_t index) { return matchData.m_operands[index]; });
      });

    return MatchData{rv::concat(
      matchData.m_instructions,
      pattern.m_initializers
        | rv::transform([&](const InstructionInitializer &initializer) {
            return Instruction::create(
              initializer.m_type, initializer.m_syntax,
              initializer.m_explicitArguments | toOperands,
              initializer.m_implicitDestinations | toOperands,
              initializer.m_implicitSources | toOperands);
          }))};
  }

  return {};
} // namespace assembly

template <typename T>
bool DagMatcher::match(
  const T &code, const ir::Placeholder &placeholder, MatchData &matchData,
  std::enable_if_t<std::is_constructible<ir::Expression, T>::value,
                   int> /*= 0*/) const {
  ir::Expression exp{code};

  using Ret    = boost::optional<Operand>;
  auto operand = [&]() -> Ret {
    switch (placeholder) {
      default:
        assert(false && "Unknown placeholder");
        return {};
      case ir::Placeholder::INT: {
        auto res = boost::get<int>(&exp);
        if (res) {
          return Operand{*res};
        }
        return {};
      }
      case ir::Placeholder::LABEL: {
        auto res = boost::get<temp::Label>(&exp);
        if (res) {
          return Operand{*res};
        }
        return {};
      }
      case ir::Placeholder::REGISTER: {
        auto res = boost::get<temp::Register>(&exp);
        if (res) {
          return Operand{*res};
        }
        return {};
      }
      case ir::Placeholder::EXPRESSION:
        return helpers::match(exp)(
          [](const temp::Register &reg) -> Ret { return Operand{reg}; },
          [](const temp::Label &label) -> Ret { return Operand{label}; },
          [&](const auto & /*default*/) -> Ret {
            auto expMatchData = match(exp);
            if (expMatchData) {
              ranges::move(expMatchData->m_instructions,
                           ranges::back_inserter(matchData.m_instructions));
              return helpers::match(exp)(
                [this](const ir::Call &) -> Ret {
                  return Operand{
                    m_codeGenerator.m_callingConvention.returnValue()};
                },
                [&matchData](const auto & /*default*/) -> Ret {
                  return Operand{
                    matchData.m_instructions.back().destinations().front()};
                });
            }

            return {};
          });
    }
  }();
  if (operand) {
    matchData.m_operands.push_back(*operand);
    return true;
  }

  return false;
}

template <typename T>
bool DagMatcher::match(const std::vector<T> &code,
                       const std::vector<T> &pattern,
                       MatchData &matchData) const {
  return code.size() == pattern.size()
         && std::equal(code.begin(), code.end(), pattern.begin(),
                       [&](const T &codeVal, const T &patternVal) {
                         return match(codeVal, patternVal, matchData);
                       });
}

bool DagMatcher::match(const std::shared_ptr<temp::Label> &code,
                       const std::shared_ptr<temp::Label> &pattern,
                       MatchData &matchData) const {
  assert(code && "code should never have an empty label");
  if (!pattern || *code == *pattern) {
    matchData.m_operands.emplace_back(*code);
    return true;
  }

  return false;
}

bool DagMatcher::match(const ir::Call &code, const ir::Call &pattern,
                       MatchData &matchData) const {
  if (!match(code.fun, pattern.fun, matchData)) {
    return false;
  }

  bool matchFailed = false;
  auto args =
    code.args | ranges::view::transform([&](const ir::Expression &arg) {
      return helpers::match(arg)(
        [](int i) -> ir::Expression { return i; },
        [](const temp::Register &reg) -> ir::Expression { return reg; },
        [](const temp::Label &label) -> ir::Expression { return label; },
        [&](const auto & /*default*/) -> ir::Expression {
          auto expMatchData = match(arg);
          if (expMatchData) {
            ranges::move(expMatchData->m_instructions,
                         ranges::back_inserter(matchData.m_instructions));
            return matchData.m_instructions.back().destinations().front();
          } else {
            matchFailed = true;
            return {};
          }
        });
    })
    | ranges::to_vector;

  if (matchFailed) {
    return false;
  }

  ranges::move(m_codeGenerator.translateArgs(args, m_tempMap),
               ranges::back_inserter(matchData.m_instructions));
  return true;
} // namespace assembly

std::string CodeGenerator::escape(const std::string &str) const {
  namespace karma = boost::spirit::karma;
  std::string res;
  using iterator = decltype(std::back_inserter(res));
  struct escaped_string : karma::grammar<iterator, std::string(char const *)> {
    escaped_string() : escaped_string::base_type(esc_str) {
      esc_char.add('\a', "\\a")('\b', "\\b")('\f', "\\f")('\n', "\\n")('\r',
                                                                       "\\r")(
        '\t', "\\t")('\v', "\\v")('\\', "\\\\")('\'', "\\\'")('"', "\\\"");
      esc_str = karma::lit(karma::_r1)
                << *(esc_char | karma::print | "\\x" << karma::hex)
                << karma::lit(karma::_r1);
    }
    karma::rule<iterator, std::string(char const *)> esc_str;
    karma::symbols<char, char const *> esc_char;
  };

  auto quote = "\"";
  karma::generate(std::back_inserter(res), escaped_string{}(quote), str);
  return res;
}

} // namespace assembly
} // namespace tiger
