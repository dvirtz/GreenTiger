#include "Assembly.h"
#include "variantMatch.h"
#include "warning_suppress.h"
MSC_DIAG_OFF(4459 4127)
#include <boost/spirit/home/x3.hpp>
MSC_DIAG_ON()
#include <range/v3/algorithm/for_each.hpp>
MSC_DIAG_OFF(4913)
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/move.hpp>
MSC_DIAG_ON()
MSC_DIAG_OFF(4459)
#include <range/v3/view/join.hpp>
MSC_DIAG_ON()
#include <gsl/span>
#include <range/v3/algorithm/for_each.hpp>
MSC_DIAG_OFF(4702)
#include <range/v3/view/concat.hpp>
MSC_DIAG_ON()
#include <range/v3/view/single.hpp>
#include <range/v3/view/transform.hpp>

namespace tiger {

namespace assembly {

static auto parse = [](const std::string &syntax, auto &&onLabel,
                       auto &&onDestination, auto &&onSource,
                       auto &&onImmediate, auto &&onOther) {
  namespace x3 = boost::spirit::x3;

  auto const callOnOperand = [](auto &&onOperand) {
    return [&](auto &ctx) { onOperand(_attr(ctx)); };
  };
  auto it = std::begin(syntax);
#ifndef NDEBUG
  auto succeeded =
#endif
    x3::parse(
      it, std::end(syntax),
      //  Begin grammar
      // clang-format off
      *(
          (x3::lit("`s") > x3::uint_[callOnOperand(std::forward<decltype(onSource)>(onSource))])
        | (x3::lit("`d") > x3::uint_[callOnOperand(std::forward<decltype(onDestination)>(onDestination))])
        | (x3::lit("`l") > x3::uint_[callOnOperand(std::forward<decltype(onLabel)>(onLabel))])
        | (x3::lit("`i") > x3::uint_[callOnOperand(std::forward<decltype(onImmediate)>(onImmediate))])
        | ((x3::lit("``") > x3::attr('`')) | (x3::char_ - '`'))[callOnOperand(std::forward<decltype(onOther)>(onOther))]
      )
      // clang-format on
      //  End grammar
    );

  assert((succeeded & it == std::end(syntax)) && "Failed to parse instruction");
};

void Instruction::print(std::ostream &out, const temp::Map &tempMap) const {
  auto const writeRegister = [&out,
                              &tempMap](const temp::Registers &registers) {
    return [&](size_t index) { out << *tempMap.lookup(registers[index]); };
  };

  auto const writeLabel = [&out](auto &&labels) {
    return [&](size_t index) { out << labels[index]; };
  };

  auto const writeImmediate = [&out](const Immediates &immediates) {
    return [&](size_t index) { out << immediates[index]; };
  };

  auto const writeOther = [&](auto other) { out << other; };

  auto const noop = [](size_t) { assert(false && "should not get here"); };

  namespace rv = ranges::views;

  helpers::match (*this)(
    [&](const Label &label) {
      parse(label.m_syntax, writeLabel(rv::single(label.m_label)), noop, noop,
            noop, writeOther);
    },
    [&](const auto &inst) {
      parse(inst.m_syntax, writeLabel(inst.m_labels),
            writeRegister(inst.m_destinations), writeRegister(inst.m_sources),
            writeImmediate(inst.m_immediates), writeOther);
    });
}

Instruction Instruction::create(const InstructionType type,
                                const std::string &syntax,
                                const Operands &operands,
                                const Operands &implicitDestinations,
                                const Operands &implicitSources) {
  size_t operandIndex = 0;
  auto setOperand     = [&](auto &toOperands) {
    return [&](size_t) {
      assert(operandIndex < operands.size() && "missing operands");
      using operand_type =
        typename std::decay_t<decltype(toOperands)>::value_type;
      assert(helpers::hasType<operand_type>(operands[operandIndex])
             && "operand's type mismatch");
      toOperands.push_back(boost::get<operand_type>(operands[operandIndex++]));
    };
  };

  auto const noop = [](auto) {};

  auto const fillInstruction = [&](auto &inst) {
    parse(inst.m_syntax, setOperand(inst.m_labels),
          setOperand(inst.m_destinations), setOperand(inst.m_sources),
          setOperand(inst.m_immediates), noop);
    auto const toRegisters = ranges::views::transform(
      [](const Operand &op) { return boost::get<temp::Register>(op); });
    inst.m_implicitDestinations = implicitDestinations | toRegisters;
    inst.m_implicitSources      = implicitSources | toRegisters;
    ranges::for_each(operands.begin() + operandIndex, operands.end(),
                     [&inst](const Operand &operand) {
                       helpers::match(operand)(
                         [&inst](int i) { inst.m_immediates.push_back(i); },
                         [&inst](const temp::Register &reg) {
                           inst.m_destinations.push_back(reg);
                         },
                         [&inst](const temp::Label &label) {
                           inst.m_labels.push_back(label);
                         });
                     });
  };

  switch (type) {
    case InstructionType::OPERATION: {
      Operation res{syntax};
      fillInstruction(res);
      return res;
    }
    case InstructionType::MOVE: {
      Move res{syntax};
      fillInstruction(res);
      return res;
    }
    case InstructionType::JUMP: {
      Jump res{syntax};
      fillInstruction(res);
      return res;
    }
    default:
      assert(type == InstructionType::LABEL && "Unknown instruction type");
      assert(operands.size() == 1 && "Labels should have a single operands");
      assert(helpers::hasType<temp::Label>(operands.front())
             && "Operands type should be a label");
      return Label{syntax, boost::get<temp::Label>(operands.front())};
  }
}

temp::Registers Instruction::destinations() const {
  return helpers::match(*this)(
    [](const Label &) -> temp::Registers { return {}; },
    [](const auto &inst) -> temp::Registers {
      return ranges::views::concat(inst.m_destinations,
                                  inst.m_implicitDestinations);
    });
}

temp::Registers Instruction::sources() const {
  return helpers::match(*this)(
    [](const Label &) -> temp::Registers { return {}; },
    [](const auto &inst) -> temp::Registers {
      return ranges::views::concat(inst.m_sources, inst.m_implicitSources);
    });
}

bool Instruction::isMove() const { return helpers::hasType<Move>(*this); }

void print(std::ostream &out, const Instructions &instructions,
           const temp::Map &tempMap) {
  ranges::for_each(instructions, [&](const Instruction &instruction) {
    instruction.print(out, tempMap);
    out << '\n';
  });
}

std::ostream &operator<<(
  std::ostream &ost,
  const std::pair<temp::Map, assembly::Instructions> &compileResult) {
  assembly::print(ost, compileResult.second, compileResult.first);
  return ost;
}

assembly::Instructions
  joinInstructions(gsl::span<const Instructions> instructions) {
  using namespace ranges;
#ifdef _MSC_VER
  assembly::Instructions res;
  for (auto &&insts : instructions) {
    move(insts, back_inserter(res));
  }
  return res;
#else
  return instructions | views::join;
#endif
}

} // namespace assembly
} // namespace tiger
