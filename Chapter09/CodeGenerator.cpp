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
#include <range/v3/algorithm/move.hpp>
MSC_DIAG_ON()
#include <range/v3/action/push_back.hpp>
#include <range/v3/view/transform.hpp>
#include <regex>
#include <sstream>

namespace tiger
{
namespace assembly
{

namespace traits
{
template <typename... Ts>
struct is_variant : std::false_type
{
};

template <typename... Ts>
struct is_variant<boost::variant<Ts...>> : std::true_type
{
};
} // namespace traits

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention,
                             Patterns &&patterns)
    : m_callingConvention{callingConvention}, m_patterns{std::move(patterns)} {}

Instructions CodeGenerator::translateFunction(const ir::Statements &statements,
                                              temp::Map &tempMap) const
{
  Instructions instructions;
  ranges::for_each(statements, [&](const ir::Statement &statement) {
    auto matched = match(statement, tempMap);
    if (!matched)
    {
      std::stringstream sst;
      sst << "Could find a match for ";
      printStatement(sst, statement, tempMap);
      throw std::logic_error{sst.str()};
    }
    ranges::move(*matched, ranges::back_inserter(instructions));
  });

  return instructions;
}

struct DagMatcher
{
  DagMatcher(const CodeGenerator &codeGenerator, temp::Map &tempMap)
      : m_codeGenerator{codeGenerator}, m_tempMap{tempMap} {}

  struct MatchData
  {
    Registers m_sources;
    Registers m_destinations;
    Labels m_labels;
    Immediates m_immediates;
    Instructions m_instructions;

    void move(MatchData &&other)
    {
      assert(other.m_sources.empty() && "Sources should have already been used");
      assert(other.m_immediates.empty() && "Immediates should have already been used");
      ranges::move(other.m_destinations, ranges::back_inserter(m_sources));
      ranges::move(other.m_labels, ranges::back_inserter(m_labels));
      ranges::move(other.m_instructions, ranges::back_inserter(m_instructions));
    }
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
             std::enable_if_t<!std::is_same<T, ir::Placeholder>::value &&
                                  boost::fusion::traits::is_sequence<T>::value,
                              int> = 0) const;

  template <typename T>
  bool match(const T &code, const T &pattern, MatchData &matchData,
             std::enable_if_t<!std::is_same<T, ir::Placeholder>::value &&
                                  !traits::is_variant<T>::value &&
                                  !boost::fusion::traits::is_sequence<T>::value,
                              int> = 0) const;

  template <typename T>
  bool
  match(const T &code, const ir::Placeholder &placeholder, MatchData &matchData,
        std::enable_if_t<std::is_constructible<ir::Expression, T>::value, int> =
            0) const;

  bool match(const ir::ConditionalJump &code, const ir::ConditionalJump &pattern,
             MatchData &matchData) const;

  bool match(const ir::Call &code, const ir::Call &pattern,
             MatchData &matchData) const;

  template <typename T, typename U>
  bool match(const T &, const U &, MatchData &) const
  {
    return false;
  }

  const CodeGenerator &m_codeGenerator;
  temp::Map &m_tempMap;
};

boost::optional<Instructions> CodeGenerator::match(const ir::Statement &statement, temp::Map &tempMap) const
{
  DagMatcher dagMatcher{*this, tempMap};
  auto dag = helpers::match(statement)(
      [](const ir::ExpressionStatement &expStatement) -> Dag {
        return expStatement.exp;
      },
      [&statement](const auto & /*default*/) -> Dag { return statement; });
  auto matchData = dagMatcher.match(dag);
  if (matchData)
  {
    return matchData->m_instructions;
  }

  return {};
}

template <typename... Ts>
bool DagMatcher::match(const boost::variant<Ts...> &code,
                       const boost::variant<Ts...> &pattern,
                       MatchData &matchData) const
{
  return helpers::match(code, pattern)(
      [&](const auto &codeVal, const auto &patternVal) {
        return this->match(codeVal, patternVal, matchData);
      });
}

template <typename T>
bool DagMatcher::match(
    const T &code, const T &pattern, MatchData &matchData,
    std::enable_if_t<!std::is_same<T, ir::Placeholder>::value &&
                         boost::fusion::traits::is_sequence<T>::value,
                     int>) const
{
  namespace bf = boost::fusion;

  return bf::all(bf::zip(code, pattern), [&](const auto &zipped) {
    return match(bf::at_c<0>(zipped), bf::at_c<1>(zipped), matchData);
  });
}

template <typename T>
bool DagMatcher::match(
    const T &code, const T &pattern, MatchData &/* matchData */,
    std::enable_if_t<!std::is_same<T, ir::Placeholder>::value &&
                         !traits::is_variant<T>::value &&
                         !boost::fusion::traits::is_sequence<T>::value,
                     int>) const
{
  return code == pattern;
}

boost::optional<DagMatcher::MatchData> DagMatcher::match(const Dag &dag) const
{
  for (const auto &pattern : m_codeGenerator.m_patterns)
  {
    MatchData matchData;
    if (!match(dag, pattern.m_dag, matchData))
    {
      continue;
    }

    auto inst = pattern.m_instruction;

    auto pDestinations = helpers::match(inst)(
        [&](Operation &op) {
          op.m_sources.swap(matchData.m_sources);
          op.m_destinations.swap(matchData.m_destinations);
          op.m_jumps.swap(matchData.m_labels);
          op.m_immediates.swap(matchData.m_immediates);
          return &op.m_destinations;
        },
        [&](Move &move) {
          move.m_sources.swap(matchData.m_sources);
          move.m_destinations.swap(matchData.m_destinations);
          move.m_labels.swap(matchData.m_labels);
          move.m_immediates.swap(matchData.m_immediates);
          return &move.m_destinations;
        },
        [&](Label &label) -> Registers * {
          assert(matchData.m_labels.size() == 1 &&
                 "Should not have more than one label");
          label.m_label = std::move(matchData.m_labels[0]);
          return nullptr;
        });

    helpers::match(dag)(
        [&](const ir::Expression &exp) {
          helpers::match(exp)(
              [&](const temp::Label &label) {
                matchData.m_labels.push_back(label);
              },
              [&](const ir::Call &) {
                matchData.m_destinations.push_back(m_codeGenerator.m_callingConvention.returnValue());
              },
              [&](const auto & /*default*/) {
                auto result = m_tempMap.newTemp();
                if (pDestinations)
                {
                  pDestinations->push_back(result);
                }
                matchData.m_destinations.push_back(result);
              });
        },
        [](const ir::Statement &) {});

    matchData.m_instructions.push_back(inst);

    return matchData;
  }

  return {};
} // namespace assembly

template <typename T>
bool DagMatcher::match(
    const T &code, const ir::Placeholder &placeholder, MatchData &matchData,
    std::enable_if_t<std::is_constructible<ir::Expression, T>::value,
                     int> /*= 0*/) const
{
  ir::Expression exp{code};

  auto const matchPlaceholder = [](const ir::Expression &exp,
                                   auto &collection) {
    return helpers::match(exp)(
        [&](const typename std::decay_t<decltype(collection)>::value_type &v) {
          collection.emplace_back(v);
          return true;
        },
        [](const auto & /*default*/) { return false; });
  };

  switch (placeholder)
  {
  default:
    assert(false && "Unknown placeholder");
    return false;
  case ir::Placeholder::INT:
    return matchPlaceholder(exp, matchData.m_immediates);
  case ir::Placeholder::LABEL:
    return matchPlaceholder(exp, matchData.m_labels);
  case ir::Placeholder::REGISTER:
    return matchPlaceholder(exp, matchData.m_sources);
  case ir::Placeholder::EXPRESSION:
    return helpers::match(exp)(
        [&matchData](const temp::Register &reg) {
          matchData.m_sources.emplace_back(reg);
          return true;
        },
        [&](const auto & /*default*/) {
          auto expMatchData = match(exp);
          if (expMatchData)
          {
            matchData.move(std::move(*expMatchData));
            return true;
          }

          return false;
        });
  }
}

template <typename T>
bool DagMatcher::match(const std::vector<T> &code,
                       const std::vector<T> &pattern,
                       MatchData &matchData) const
{
  return code.size() == pattern.size() &&
         std::equal(code.begin(), code.end(), pattern.begin(),
                    [&](const T &codeVal, const T &patternVal) {
                      return match(codeVal, patternVal, matchData);
                    });
}

bool DagMatcher::match(const ir::ConditionalJump &code, const ir::ConditionalJump &pattern,
                       MatchData &matchData) const
{
  matchData.m_labels.push_back(*code.trueDest);
  return match(code.op, pattern.op, matchData) &&
         match(code.left, pattern.left, matchData) && match(code.right, pattern.right, matchData);
}

bool DagMatcher::match(const ir::Call &code, const ir::Call &pattern,
                       MatchData &matchData) const
{
  if (!match(code.fun, pattern.fun, matchData))
  {
    return false;
  }

  bool matchFailed = false;
  auto args = code.args | ranges::view::transform([&](const ir::Expression &arg) {
                return helpers::match(arg)(
                    [](int i) -> ir::Expression {
                      return i;
                    },
                    [](const temp::Register &reg) -> ir::Expression {
                      return reg;
                    },
                    [](const temp::Label &label) -> ir::Expression {
                      return label;
                    },
                    [&](const auto & /*default*/) -> ir::Expression {
                      auto expMatchData = match(arg);
                      if (expMatchData)
                      {
                        ranges::move(expMatchData->m_instructions, ranges::back_inserter(matchData.m_instructions));
                        return expMatchData->m_destinations[0];
                      }
                      else
                      {
                        matchFailed = true;
                        return {};
                      }
                    });
              }) |
              ranges::to_vector;

  if (matchFailed)
  {
    return false;
  }

  ranges::move(m_codeGenerator.translateArgs(args, m_tempMap),
               ranges::back_inserter(matchData.m_instructions));
  ranges::push_back(matchData.m_destinations, m_codeGenerator.m_callingConvention.callDefinedRegisters());
  return true;
} // namespace assembly

std::string CodeGenerator::escape(const std::string &str) const
{
  namespace karma = boost::spirit::karma;
  std::string res;
  using iterator = decltype(std::back_inserter(res));
  struct escaped_string : karma::grammar<iterator, std::string(char const *)>
  {
    escaped_string() : escaped_string::base_type(esc_str)
    {
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
