#pragma once
#include "Assembly.h"
#include "Tree.h"
#include <boost/optional/optional_fwd.hpp>
#include <utility>
#include <vector>

namespace tiger {

namespace frame {
class CallingConvention;
}

namespace assembly {

using Dag       = boost::variant<ir::Expression, ir::Statement>;
using Arguments = std::vector<boost::variant<temp::Register, size_t>>;
struct InstructionInitializer {
  InstructionType m_type;
  std::string m_syntax;
  Arguments m_explicitArguments;
  Arguments m_implicitSources;
  Arguments m_implicitDestinations;
};

struct Pattern {
  Dag m_dag;
  std::vector<InstructionInitializer> m_initializers;
};

inline ir::Placeholder label() { return ir::Placeholder::LABEL; }

inline ir::Placeholder reg() { return ir::Placeholder::REGISTER; }

inline ir::Placeholder imm() { return ir::Placeholder::INT; }

inline ir::Placeholder exp() { return ir::Placeholder::EXPRESSION; }

using Patterns = std::vector<Pattern>;

class CodeGenerator {
public:
  CodeGenerator(frame::CallingConvention &callingConvention,
                Patterns &&patterns);

  Instructions translateFunction(const ir::Statements &statements,
                                 temp::Map &tempMap) const;

  virtual Instructions translateString(const temp::Label &label,
                                       const std::string &string,
                                       temp::Map &tempMap) = 0;

protected:
  std::string escape(const std::string &str) const;

  frame::CallingConvention &m_callingConvention;

private:
  boost::optional<Instructions> match(const ir::Statement &statement,
                                      temp::Map &tempMap) const;

  virtual Instructions translateArgs(const std::vector<ir::Expression> &args,
                                     const temp::Map &tempMap) const = 0;

  friend struct DagMatcher;

  Patterns m_patterns;
};

} // namespace assembly
} // namespace tiger
