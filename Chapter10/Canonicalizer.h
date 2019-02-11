#pragma once
#include "Fragment.h"
#include "Tree.h"
#include "type_traits.h"

namespace tiger {

class Canonicalizer {
public:
  using BasicBlocks = std::vector<ir::Statements>;

  Canonicalizer(temp::Map &tempMap);

  // reduce program to a list of statements
  ir::Statements canonicalize(ir::Statement &&statement);

private:
  // removes the ESEQs and moves the CALLs to top level
  ir::Statements linearize(ir::Statement &&statement) const;

  // groups statements into sequences of straight-line code
  BasicBlocks makeBasicBlocks(ir::Statements &&statements) const;

  // orders the blocks so that every CJUMP is followed by
  // its false label
  ir::Statements traceSchedule(BasicBlocks &&block) const;

  // estimates (very naively) whether stm and exp commute
  bool commutes(const ir::Statement &stm, const ir::Expression &exp) const;

  // pull all the ir::ExpressionSequence-s out of a list of expressions
  // and combine the statement parts of these ir::ExpressionSequence-s into one
  // big ir::Statement
  template <typename T, typename... Ts>
  std::enable_if_t<!helpers::is_iterator<T>::value, ir::Statement>
    reorder(T &t, Ts &... ts) const;

  template <typename Iterator,
            typename = std::enable_if_t<helpers::is_iterator<Iterator>::value>>
  ir::Statement reorder(Iterator first, Iterator last) const;

  ir::Statement reorderStatement(ir::Statement &stm) const;

  struct Reordered {
    ir::Statement m_stm;
    ir::Expression m_exp;
  };

  Reordered reorderExpression(ir::Expression &exp) const;

  bool isConst(const ir::Expression &expression) const;

  bool hasNoSideEffects(const ir::Expression &expression) const;

  bool isNop(const ir::Statement &stm) const;

  template <typename Sequence,
            typename = std::enable_if_t<helpers::is_sequence<Sequence>::value>>
  ir::Statement sequence(const Sequence &statements) const;

  template <typename Statement, typename... Statements>
  std::enable_if_t<!helpers::is_sequence<Statement>::value, ir::Statement>
    sequence(const Statement &statement,
             const Statements &... statements) const;

  ir::Statements linearizeHelper(ir::Statement &statement) const;

  ir::Statements concatenate(const ir::Statements &left,
                             const ir::Statements &right) const;

  temp::Map &m_tempMap;
};

} // namespace tiger
