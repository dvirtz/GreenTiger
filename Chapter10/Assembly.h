#pragma once
#include "TempMap.h"
#include <boost/variant.hpp>
#include <gsl/span>
#include <gsl/string_span>
#include <ostream>
#include <string>
#include <vector>

namespace tiger {

namespace assembly {
using Immediates = std::vector<int>;

enum class InstructionType { OPERATION, JUMP, MOVE, LABEL };

template <InstructionType> struct InstructionBase {
  std::string m_syntax;
  temp::Registers m_destinations;
  temp::Registers m_sources;
  temp::Labels m_labels;
  Immediates m_immediates;
  temp::Registers m_implicitDestinations;
  temp::Registers m_implicitSources;
};

template <> struct InstructionBase<InstructionType::LABEL> {
  std::string m_syntax;
  temp::Label m_label;
};

using Operation = InstructionBase<InstructionType::OPERATION>;
using Move      = InstructionBase<InstructionType::MOVE>;
using Jump      = InstructionBase<InstructionType::JUMP>;
using Label     = InstructionBase<InstructionType::LABEL>;

using Operand  = boost::variant<temp::Register, temp::Label, int>;
using Operands = std::vector<Operand>;

class Instruction : public boost::variant<Operation, Label, Move, Jump> {
  using base = boost::variant<Operation, Label, Move, Jump>;

public:
  using base::base;

  void print(std::ostream &out, const temp::Map &tempMap) const;

  static Instruction create(const InstructionType type,
                            const std::string &syntax, const Operands &operands,
                            const Operands &implicitDestinations,
                            const Operands &implicitSources);

  temp::Registers destinations() const;
  temp::Registers sources() const;
  bool isMove() const;
};

using Instructions = std::vector<Instruction>;

void print(std::ostream &out, const Instructions &instructions,
           const temp::Map &tempMap);

std::ostream &
  operator<<(std::ostream &ost,
             const std::pair<temp::Map, assembly::Instructions> &compileResult);

Instructions joinInstructions(gsl::span<const Instructions> instructions);

struct Procedure {
  std::string m_prolog;
  Instructions m_body;
  std::string m_epilog;
};
} // namespace assembly
} // namespace tiger