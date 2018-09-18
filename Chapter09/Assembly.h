#pragma once
#include "TempMap.h"
#include <boost/variant.hpp>
#include <ostream>
#include <string>
#include <vector>
#include <range/v3/view/any_view.hpp>

namespace tiger {

namespace assembly {
using Registers = std::vector<temp::Register>;
using Labels = std::vector<temp::Label>;
using Immediates = std::vector<int>;

struct Operation {
  std::string m_syntax;
  Registers m_destinations;
  Registers m_sources;
  Labels m_jumps;
  Immediates m_immediates;

  Operation(std::initializer_list<std::string> syntaxes, 
            const Registers &destinations = {}, 
            const Registers &sources = {}, 
            const Labels &labels = {}, 
            const Immediates &immediates = {});
};

struct Label {
  std::string m_syntax;
  temp::Label m_label;
};

struct Move {
  std::string m_syntax;
  Registers m_destinations;
  Registers m_sources;
  Labels m_labels;
  Immediates m_immediates;
};

struct Instruction : public boost::variant<Operation, Label, Move> {
  using base = boost::variant<Operation, Label, Move>;
  using base::base;
  using base::operator=;

  void print(std::ostream &out, const temp::Map &tempMap) const;
};

using Instructions = std::vector<Instruction>;

void print(std::ostream &out, const Instructions &instructions,
           const temp::Map &tempMap);

std::ostream &
operator<<(std::ostream &ost,
           const std::pair<temp::Map, assembly::Instructions> &compileResult);

Instructions
joinInstructions(ranges::any_view<Instructions> instructions);

struct Procedure {
  std::string m_prolog;
  Instructions m_body;
  std::string m_epilog;
};
} // namespace assembly
} // namespace tiger