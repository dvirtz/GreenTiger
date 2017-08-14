#include "Program.h"
#include <iostream>

namespace GreenTiger {

size_t maxargs(const Statement &statement) {
  size_t result = 0;
  std::function<void(Expression)> maxargsExpression =
      [&](const Expression &expression) {
        helpers::match(expression)(
            [](const Variable &) {}, [](const Number &) {},
            [&](const BinaryOperation &bo) {
              maxargsExpression(bo.m_lhs);
              maxargsExpression(bo.m_rhs);
            },
            [&](const ExpressionSequence &es) {
              result = std::max(result, maxargs(es.m_statement));
              maxargsExpression(es.m_expression);
            });
      };

  helpers::match(statement)(
      [&](const CompoundStatement &cs) {
        result = std::max(
            {result, maxargs(cs.m_statement1), maxargs(cs.m_statement2)});
      },
      [&](const AssignmentStatement &as) {
        maxargsExpression(as.m_expression);
      },
      [&](const PrintStatement &ps) {
        result = std::max(result, ps.m_expressions.size());
        for (auto &expression : ps.m_expressions) {
          maxargsExpression(expression);
        }
      });

  return result;
}

void interp(const Statement &statement) {
  VariableValues values;
  return interpStm(statement, values);
}

void interpStm(const Statement &statement, VariableValues &values) {
  helpers::match(statement)(
      [&](const CompoundStatement &cs) {
        interpStm(cs.m_statement1, values);
        interpStm(cs.m_statement2, values);
      },
      [&](const AssignmentStatement &as) {
        values[as.m_variable] = interpExp(as.m_expression, values);
      },
      [&](const PrintStatement &ps) {
        for (auto &expression : ps.m_expressions) {
          std::cout << interpExp(expression, values) << '\n';
        }
      });
}

int interpExp(const Expression &expression, VariableValues &values) {
  return helpers::match(expression)(
      [&](const Variable &v) { return values[v]; },
      [](const Number &n) { return n; },
      [&](const BinaryOperation &bo) {
        auto evaluate = [](int lhs, BinOp op, int rhs) {
          switch (op) {
          case BinOp::PLUS:
            return lhs + rhs;
          case BinOp::MINUS:
            return lhs - rhs;
          case BinOp::TIMES:
            return lhs * rhs;
          case BinOp::DIV:
            return lhs / rhs;
          default:
            assert(false && "Unknown operation");
            return 0;
          }
        };

        return evaluate(interpExp(bo.m_lhs, values), bo.m_operation,
                        interpExp(bo.m_rhs, values));
      },
      [&](const ExpressionSequence &es) {
        interpStm(es.m_statement, values);
        return interpExp(es.m_expression, values);
      });
}

} // namespace GreenTiger