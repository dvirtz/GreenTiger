#pragma once
#include "Fragment.h"
#include "Frame.h"
#include "Tree.h"
#include <boost/optional.hpp>

namespace tiger {

namespace frame {
class CallingConvention;
}

namespace translator {

using PatchList = std::vector<std::reference_wrapper<temp::Label>>;

struct Condition {
  PatchList m_trues, m_falses;
  ir::Statement m_statement;
};

using Expression = boost::variant<ir::Expression, ir::Statement, Condition>;

using Level = size_t;

struct VariableAccess {
  Level level;
  frame::VariableAccess frameAccess;
};

class Translator {
public:
  Translator(temp::Map &tempMap, frame::CallingConvention &callingConvention);

  Level outermost() const;
  Level newLevel(temp::Label label, const frame::BoolList &formals);
  std::vector<VariableAccess> formals(Level level);
  VariableAccess allocateLocal(Level level, bool escapes);

  ir::Expression toExpression(const Expression &exp);
  ir::Statement toStatement(const Expression &exp);
  Condition toCondition(const Expression &exp);

  Expression translateVar(const std::vector<Level> &nestingLevels,
                          const VariableAccess &access);

  Expression translateArrayAccess(const Expression &array,
                                  const Expression &index);

  Expression translateArithmetic(ir::BinOp operation, const Expression &lhs,
                                 const Expression &rhs);

  Expression translateRelation(ir::RelOp relation, const Expression &lhs,
                               const Expression &rhs);

  Expression translateStringCompare(ir::RelOp relation, const Expression &lhs,
                                    const Expression &rhs);

  Expression
    translateConditional(const Expression &test, const Expression &thenExp,
                         const boost::optional<Expression> &elseExp = {});

  Expression translateConstant(int value);

  Expression translateString(const std::string &value);

  Expression translateRecord(const std::vector<Expression> &fields);

  Expression translateArray(const Expression &size, const Expression &value);

  temp::Label loopDone();

  Expression translateWhileLoop(const Expression &test, const Expression &body,
                                const temp::Label &loopDone);

  Expression translateBreak(const temp::Label &loopDone);

  Expression translateForLoop(const Expression &var, const Expression &from,
                              const Expression &to, const Expression &body,
                              const temp::Label &loopDone);

  Expression translateCall(const std::vector<Level> &nestingLevels,
                           const temp::Label &functionLabel,
                           Level functionLevel,
                           const std::vector<Expression> &arguments);

  Expression translateVarDecleration(const Expression &access,
                                     const Expression &init);

  Expression translateLet(const std::vector<Expression> &declarations,
                          const std::vector<Expression> &body);

  void translateFunction(Level level, const temp::Label &label,
                         const Expression &body);

  Expression translateAssignment(const Expression &var, const Expression &exp);

  FragmentList result() const;

private:
  void doPatch(const PatchList &patchList, const temp::Label &label);

  ir::Expression framePointer(const std::vector<Level> &nestingLevels,
                              Level level);

  temp::Map &m_tempMap;
  frame::CallingConvention &m_callingConvention;
  std::vector<std::shared_ptr<frame::Frame>> m_frames;
  FragmentList m_fragments;
  const int m_wordSize;
  Level m_outermost;
};

} // namespace translator
} // namespace tiger
