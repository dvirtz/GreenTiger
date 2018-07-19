#include "Assembly.h"
#include "variantMatch.h"
#include <boost/spirit/home/x3.hpp>
#include <range/v3/algorithm/for_each.hpp>
#ifdef _MSC_VER
#include <range/v3/algorithm/move.hpp>
#else
#include <range/v3/action/join.hpp>
#endif
#include <gsl/span>

namespace tiger {

namespace assembly {

static void format(std::ostream &out, const std::string &syntax,
                   const temp::Map &tempMap, const gsl::span<const temp::Label> labels = {},
                   const gsl::span<const temp::Register> destinations = {},
                   const gsl::span<const temp::Register> sources = {}, 
                   const gsl::span<const int> immediates = {}) {

  using namespace boost::spirit::x3;

  auto const lookup = helpers::overload(
    [&](const temp::Register &reg) { return *tempMap.lookup(reg); },
    [](const temp::Label &label) { return label.get(); },
    [](const int i) { return i; }
  );

  auto const writeOperand = [&](auto &operands) {
    return [&](auto &ctx) { out << lookup(operands[_attr(ctx)]); };
  };

  auto succeeded =
      parse(std::begin(syntax), std::end(syntax),

                   //  Begin grammar
                   // clang-format off
      *(
          (lit("`s") > int_[writeOperand(sources)])
        | (lit("`d") > int_[writeOperand(destinations)])
        | (lit("`l") > int_[writeOperand(labels)])
        | (lit("`i") > int_[writeOperand(immediates)])
        | lit("``")[([&](auto & /*ctx*/) { out << '`'; })]
        | (char_ - '`')[([&](auto &ctx) { out << _attr(ctx); })]
      )
                   // clang-format on
                   //  End grammar
      );

  assert(succeeded && "Failed to parse instruction");
}

void Instruction::print(std::ostream &out, const temp::Map &tempMap) const {
  helpers::match (*this)(
      [&](const Operation &operation) {
        format(out, operation.m_syntax, tempMap, operation.m_jumps, operation.m_destinations,
               operation.m_sources, operation.m_immediates);
      },
      [&](const Label &label) { format(out, label.m_syntax, tempMap, {&label.m_label, 1}); },
      [&](const Move &move) {
        format(out, move.m_syntax, tempMap, move.m_labels, move.m_destinations,
               move.m_sources, move.m_immediates);
      });
}

void print(std::ostream &out, const Instructions &instructions,
           const temp::Map &tempMap) {
  ranges::for_each(instructions, [&](const Instruction &instruction) {
    instruction.print(out, tempMap);
    out << '\n';
  });
}

std::ostream &
operator<<(std::ostream &ost,
           const std::pair<temp::Map, assembly::Instructions> &compileResult) {
  assembly::print(ost, compileResult.second, compileResult.first);
  return ost;
}

assembly::Instructions
joinInstructions(ranges::any_view<Instructions> instructions) {
  using namespace ranges;
#ifdef _MSC_VER
  assembly::Instructions res;
  for (auto &&insts : instructions) {
    move(insts, back_inserter(res));
  }
  return res;
#else
  return action::join(instructions);
#endif
}

} // namespace assembly
} // namespace tiger
