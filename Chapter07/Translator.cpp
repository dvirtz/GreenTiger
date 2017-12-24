#include "Translator.h"
#include "CallingConvention.h"
#include "variantMatch.h"
#include <boost/dynamic_bitset.hpp>
#include <cassert>

namespace tiger {

namespace translator {

Translator::Translator(temp::Map &tempMap,
                       frame::CallingConvention &callingConvention)
    : m_tempMap(tempMap), m_callingConvention(callingConvention),
      m_wordSize(m_callingConvention.wordSize()),
      m_outermost(newLevel(temp::Label{"start"}, frame::BoolList{})) {}

Level Translator::outermost() const { return m_outermost; }

Level Translator::newLevel(temp::Label label, const frame::BoolList &formals) {
  // add static link
  frame::BoolList withStaticLink = formals;
  withStaticLink.resize(withStaticLink.size() + 1);
  (withStaticLink <<= 1).set(0, true);
  m_frames.emplace_back(m_callingConvention.createFrame(label, withStaticLink));
  return m_frames.size() - 1;
}

std::vector<VariableAccess> Translator::formals(Level level) {
  std::vector<VariableAccess> res;
  auto formals = m_frames[level]->formals();
  res.reserve(formals.size());
  std::transform(formals.begin(), formals.end(), std::back_inserter(res),
                 [level](const frame::VariableAccess &access) {
                   return VariableAccess{level, access};
                 });
  return res;
}

VariableAccess Translator::allocateLocal(Level level, bool escapes) {
  return {level, m_frames[level]->allocateLocal(escapes)};
}

ir::Expression Translator::toExpression(const Expression &exp) {
  using helpers::match;
  return match(exp)([](const ir::Expression &e) { return e; },
                    [](const ir::Statement &stm) -> ir::Expression {
                      return ir::ExpressionSequence{stm, 0};
                    },
                    [this](const Condition &cond) -> ir::Expression {
                      auto r = m_tempMap.newTemp();
                      auto t = m_tempMap.newLabel(), f = m_tempMap.newLabel();
                      doPatch(cond.m_trues, t);
                      doPatch(cond.m_falses, f);
                      return ir::ExpressionSequence{
                          ir::Sequence{ir::Move{r, 1}, cond.m_statement, f,
                                       ir::Move{r, 0}, t},
                          r};
                    });
}

tiger::ir::Statement Translator::toStatement(const Expression &exp) {
  using helpers::match;
  return match(exp)(
      [](const ir::Expression &e) -> ir::Statement {
        return ir::ExpressionStatement{e};
      },
      [](const ir::Statement &stm) { return stm; },
      [this](const Condition &cond) { return cond.m_statement; });
}

Condition Translator::toCondition(const Expression &exp) {
  using helpers::match;
  return match(exp)(
      [this](const ir::Expression &e) {
        auto cjump = ir::ConditionalJump{ir::RelOp::NE, e, 0,
                                         std::make_shared<temp::Label>(),
                                         std::make_shared<temp::Label>()};
        return Condition{{*cjump.trueDest}, {*cjump.falseDest}, cjump};
      },
      [](const ir::Statement &stm) {
        assert(false && "Can't convert a statement to a condition");
        return Condition{};
      },
      [this](const Condition &cond) { return cond; });
}

Expression Translator::translateVar(const std::vector<Level> &nestingLevels,
                                    const VariableAccess &access) {
  return m_callingConvention.accessFrame(
      access.frameAccess, framePointer(nestingLevels, access.level));
}

Expression Translator::translateArrayAccess(const Expression &array,
                                            const Expression &index) {
  return ir::BinaryOperation{
      ir::BinOp::PLUS, toExpression(array),
      ir::BinaryOperation{ir::BinOp::MUL, toExpression(index), m_wordSize}};
}

Expression Translator::translateArithmetic(ir::BinOp operation,
                                           const Expression &lhs,
                                           const Expression &rhs) {
  return ir::BinaryOperation{operation, toExpression(lhs), toExpression(rhs)};
}

Expression Translator::translateRelation(ir::RelOp relation,
                                         const Expression &lhs,
                                         const Expression &rhs) {
  ir::ConditionalJump cjump{relation, toExpression(lhs), toExpression(rhs),
                            std::make_unique<temp::Label>(),
                            std::make_unique<temp::Label>()};
  return Condition{PatchList{*cjump.trueDest}, PatchList{*cjump.falseDest},
                   cjump};
}

Expression Translator::translateStringCompare(ir::RelOp relation,
                                              const Expression &lhs,
                                              const Expression &rhs) {
  ir::ConditionalJump cjump{
      relation,
      m_callingConvention.externalCall("stringCompare",
                                       {toExpression(lhs), toExpression(rhs)}),
      0, std::make_unique<temp::Label>(), std::make_unique<temp::Label>()};
  return Condition{PatchList{*cjump.trueDest}, PatchList{*cjump.falseDest},
                   cjump};
}

Expression
Translator::translateConditional(const Expression &test,
                                 const Expression &thenExp,
                                 const boost::optional<Expression> &elseExp) {
  auto t = m_tempMap.newLabel();
  auto f = m_tempMap.newLabel();
  auto join = m_tempMap.newLabel();
  auto r = m_tempMap.newTemp();
  auto testCond = toCondition(test);

  bool rUsed = false;

  auto translateBranch = [&](const Expression &translated, temp::Label &label,
                             PatchList &patchList) {
    using helpers::match;

    doPatch(patchList, label);
    ir::Sequence res;

    match(translated)(
        [&](const ir::Statement &statement) {
          res.statements.emplace_back(label);
          res.statements.emplace_back(statement);
        },
        [&](const auto &exp) {
          rUsed = true;
          res.statements.emplace_back(label);
          res.statements.emplace_back(ir::Move{r, this->toExpression(exp)});
        });

    res.statements.emplace_back(ir::Jump{join, {join}});

    return res;
  };

  auto thenStatement = translateBranch(thenExp, t, testCond.m_trues);
  ir::Sequence res{testCond.m_statement, thenStatement};
  if (elseExp) {
    auto elseStatement = translateBranch(*elseExp, f, testCond.m_falses);
    res.statements.emplace_back(elseStatement);
  }
  res.statements.emplace_back(join);

  if (rUsed) {
    return ir::ExpressionSequence{res, r};
  }

  return res;
}

Expression Translator::translateConstant(int value) {
  return ir::Expression{value};
}

Expression Translator::translateString(const std::string &value) {
  auto lab = m_tempMap.newLabel();
  m_callingConvention.allocateString(lab, value);
  return ir::Expression{lab};
}

Expression Translator::translateRecord(const std::vector<Expression> &fields) {
  auto r = m_tempMap.newTemp();
  ir::Sequence res;
  res.statements.emplace_back(
      ir::Move{r, m_callingConvention.externalCall(
                      "malloc", {ir::Expression{m_wordSize * fields.size()}})});
  for (const auto &field : fields) {
    auto address = ir::BinaryOperation{
        ir::BinOp::PLUS, r,
        ir::BinaryOperation{ir::BinOp::MUL,
                            std::distance(fields.data(), &field), m_wordSize}};
    res.statements.emplace_back(
        ir::Move{ir::MemoryAccess{address}, toExpression(field)});
  }
  return ir::ExpressionSequence{res, r};
}

Expression Translator::translateArray(const Expression &size,
                                      const Expression &value) {
  auto r = m_tempMap.newTemp();
  ir::Sequence res{
      ir::Move{r, m_callingConvention.externalCall(
                      "malloc", {ir::BinaryOperation{ir::BinOp::MUL, m_wordSize,
                                                     toExpression(size)}})},
      ir::ExpressionStatement{m_callingConvention.externalCall(
          "initArray", {toExpression(size), toExpression(value)})}};
  return ir::ExpressionSequence{res, r};
}

temp::Label Translator::loopDone() { return m_tempMap.newLabel(); }

Expression Translator::translateWhileLoop(const Expression &test,
                                          const Expression &body,
                                          const temp::Label &loopDone) {
  auto loopStart = m_tempMap.newLabel();

  return ir::Sequence{
      loopStart,
      ir::ConditionalJump{ir::RelOp::EQ, toExpression(test), 0,
                          std::make_shared<temp::Label>(loopDone)},
      toStatement(body), ir::Jump{loopStart, {loopStart}}, loopDone};
}

Expression Translator::translateBreak(const temp::Label &loopDone) {
  return ir::Jump{loopDone};
}

Expression Translator::translateForLoop(const Expression &from,
                                        const Expression &to,
                                        const Expression &body,
                                        const temp::Label &loopDone) {
  auto i = m_tempMap.newTemp();
  auto limit = m_tempMap.newTemp();
  auto loopStart = m_tempMap.newLabel();

  return ir::Sequence{
      ir::Move{i, toExpression(from)},
      ir::Move{limit, toExpression(to)},
      ir::ConditionalJump{ir::RelOp::GT, i, limit,
                          std::make_shared<temp::Label>(loopDone)},
      loopStart,
      toStatement(body),
      ir::Move{i, ir::BinaryOperation{ir::BinOp::PLUS, i, 1}},
      ir::ConditionalJump{ir::RelOp::LT, i, limit,
                          std::make_shared<temp::Label>(loopStart)},
      loopDone};
}

Expression Translator::translateCall(const std::vector<Level> &nestingLevels,
                                     const temp::Label &functionLabel,
                                     Level functionLevel,
                                     const std::vector<Expression> &arguments) {
  auto staticLink = m_callingConvention.accessFrame(
      m_frames[functionLevel]->formals().front(),
      framePointer(nestingLevels, functionLevel));
  std::vector<ir::Expression> argExpressions{staticLink};
  argExpressions.reserve(arguments.size() + 1);
  std::transform(arguments.begin(), arguments.end(),
                 std::back_inserter(argExpressions),
                 [this](const Expression &arg) { return toExpression(arg); });
  return ir::Call{functionLabel, argExpressions};
}

Expression Translator::translateVarDecleration(const Expression &access,
                                               const Expression &init) {
  return ir::Move{toExpression(access), toExpression(init)};
}

Expression Translator::translateLet(const std::vector<Expression> &declarations,
                                    const std::vector<Expression> &body) {
  ir::ExpressionSequence res;
  ir::Sequence statements;
  statements.statements.reserve(declarations.size() + body.size());
  std::transform(declarations.begin(), declarations.end(),
                 std::back_inserter(statements.statements),
                 [this](const Expression &dec) { return toStatement(dec); });
  // the last expression is the result
  if (body.empty() == false) {
    std::transform(body.begin(), std::prev(body.end()),
                   std::back_inserter(statements.statements),
                   [this](const Expression &exp) { return toStatement(exp); });
    res.exp = toExpression(body.back());
  }

  res.stm = statements;

  return res;
}

Expression Translator::translateFunctions(
    Level level,
    const std::vector<std::pair<temp::Label, Expression>> &functions) {
  ir::Sequence res;
  res.statements.reserve(functions.size());
  std::transform(
      functions.begin(), functions.end(), std::back_inserter(res.statements),
      [ this, &frame = *m_frames[level] ](
          const std::pair<temp::Label, Expression> &func) {
        ir::Move augmentedBody{m_callingConvention.returnValue(),
                               toExpression(func.second)};
        return ir::Sequence{func.first, frame.procEntryExit1(augmentedBody)};
      });

  return res;
}

Expression Translator::translateAssignment(const Expression &var,
                                           const Expression &exp) {
  auto varExp = toExpression(var);
  return ir::Move{varExp, toExpression(exp)};
}

void Translator::doPatch(const PatchList &patchList, const temp::Label &label) {
  for (temp::Label &patch : patchList) {
    patch = label;
  }
}

ir::Expression Translator::framePointer(const std::vector<Level> &nestingLevels,
                                        Level level) {
  // strip off static links until deceleration level is reached
  auto levelIt = nestingLevels.rbegin();
  ir::Expression res = m_callingConvention.framePointer();
  for (; levelIt != nestingLevels.rend() && *levelIt != level; ++levelIt) {
    const auto &frame = *m_frames[*levelIt];
    auto staticLink = frame.formals().front();
    res = m_callingConvention.accessFrame(staticLink, res);
  }
  assert(levelIt != nestingLevels.rend() && "variable access level not found");
  return res;
}

} // namespace translator
} // namespace tiger
