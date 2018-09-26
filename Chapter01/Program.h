#pragma once
#include "variantMatch.h"
#include <boost/variant.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace GreenTiger {
using Variable = std::string;
using Number   = int;

struct CompoundStatement;
struct AssignmentStatement;
struct PrintStatement;
struct BinaryOperation;
struct ExpressionSequence;

using Statement = boost::variant<boost::recursive_wrapper<CompoundStatement>,
                                 boost::recursive_wrapper<AssignmentStatement>,
                                 boost::recursive_wrapper<PrintStatement>>;
using Expression =
  boost::variant<Variable, Number, boost::recursive_wrapper<BinaryOperation>,
                 boost::recursive_wrapper<ExpressionSequence>>;

struct CompoundStatement {
  Statement m_statement1;
  Statement m_statement2;

  CompoundStatement(const Statement &statement1, const Statement &statement2) :
      m_statement1(statement1), m_statement2(statement2) {}
};

struct AssignmentStatement {
  Variable m_variable;
  Expression m_expression;

  AssignmentStatement(const Variable &variable, const Expression &expression) :
      m_variable(variable), m_expression(expression) {}
};

struct PrintStatement {
  std::vector<Expression> m_expressions;

  template <typename... Args>
  PrintStatement(Args &&... args) :
      m_expressions({std::forward<Args>(args)...}) {}
};

enum class BinOp { PLUS, MINUS, TIMES, DIV };

struct BinaryOperation {
  Expression m_lhs;
  BinOp m_operation;
  Expression m_rhs;

  BinaryOperation(const Expression &lhs, BinOp operation,
                  const Expression &rhs) :
      m_lhs(lhs),
      m_operation(operation), m_rhs(rhs) {}
};
struct ExpressionSequence {
  Statement m_statement;
  Expression m_expression;

  ExpressionSequence(const Statement &statement, const Expression &expression) :
      m_statement(statement), m_expression(expression) {}
};

size_t maxargs(const Statement &statement);
void interp(const Statement &statement);

using VariableValues = std::unordered_map<Variable, int>;
void interpStm(const Statement &statement, VariableValues &values);
int interpExp(const Expression &expression, VariableValues &values);
} // namespace GreenTiger