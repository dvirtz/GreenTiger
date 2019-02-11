#pragma once
#include "TempMap.h"
#include "variantMatch.h"
#include <boost/optional.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <iosfwd>
#include <vector>

// definitions for intermediate representation (IR) trees.

namespace tiger {

namespace ir {

enum class BinOp {
  PLUS,
  MINUS,
  MUL,
  DIV,
  AND,
  OR,
  LSHIFT,
  RSHIFT,
  ARSHIFT,
  XOR
};

enum class RelOp { EQ, NE, LT, GT, LE, GE, ULT, ULE, UGT, UGE };

struct Sequence;
struct Jump;
struct ConditionalJump;
struct Move;
struct ExpressionStatement;

enum class Placeholder { INT, LABEL, REGISTER, EXPRESSION };

using Statement =
  boost::variant<boost::recursive_wrapper<Sequence>, temp::Label,
                 boost::recursive_wrapper<Jump>,
                 boost::recursive_wrapper<ConditionalJump>,
                 boost::recursive_wrapper<Move>,
                 boost::recursive_wrapper<ExpressionStatement>, Placeholder>;

struct BinaryOperation;
struct ExpressionSequence;
struct Call;
struct MemoryAccess;

using Expression = boost::variant<int, temp::Label, temp::Register,
                                  boost::recursive_wrapper<BinaryOperation>,
                                  boost::recursive_wrapper<MemoryAccess>,
                                  boost::recursive_wrapper<ExpressionSequence>,
                                  boost::recursive_wrapper<Call>, Placeholder>;

using Statements = std::vector<Statement>;

struct Sequence {
  Statements statements;

  // cannot make the initializer_list ctor default because of
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60437
  Sequence() = default;
  Sequence(std::initializer_list<Statement> statements) :
      statements{statements} {}
  template <typename U>
  Sequence(U &&statements) : statements{std::forward<U>(statements)} {}
};

struct Jump {
  Expression exp;
  std::vector<temp::Label> jumps;

  Jump(const temp::Label &label) : exp{label}, jumps{label} {}
  Jump(Placeholder placeholder) : exp{placeholder} {}
};

struct ConditionalJump {
  RelOp op;
  Expression left, right;
  // using shared_ptr as boost::variant 1.65 still doesn't handle move only
  // types correctly see https://svn.boost.org/trac10/ticket/6971
  std::shared_ptr<temp::Label> trueDest, falseDest;
  ConditionalJump(RelOp op, const Expression &left, const Expression &right,
                  const temp::Label &trueDest  = {},
                  const temp::Label &falseDest = {}) :
      op{op},
      left{left}, right{right}, trueDest{std::make_shared<temp::Label>(
                                  trueDest)},
      falseDest{std::make_shared<temp::Label>(falseDest)} {}

  ConditionalJump(RelOp op, const Expression &left, const Expression &right,
                  Placeholder /*trueDest*/, Placeholder /*falseDest*/) :
      op{op},
      left{left}, right{right} {}
};

struct Move {
  Expression src, dst;
};

struct ExpressionStatement {
  Expression exp;
};

struct BinaryOperation {
  BinOp op;
  Expression left, right;
};

struct MemoryAccess {
  Expression address;
};

struct ExpressionSequence {
  Statement stm;
  Expression exp;
};

struct Call {
  Expression fun;
  std::vector<Expression> args;
};

/* printers */

std::ostream &operator<<(std::ostream &ost, BinOp binOp);

std::ostream &operator<<(std::ostream &ost, RelOp relOp);

std::ostream &operator<<(std::ostream &ost, Placeholder placeholder);

void printIndentation(std::ostream &ost, int indentation, bool closing = false);

void printStatement(std::ostream &ost, const Statement &statement,
                    const temp::Map &tempMap, const int indent = 0);

void printExpression(std::ostream &ost, const Expression &expression,
                     const temp::Map &tempMap, const int indent = 0);

void printStatements(std::ostream &ost, const Statements &statements,
                     const temp::Map &tempMap, const int indent = 0);

void dump(const Expression &expression, const temp::Map &tempMap);
void dump(const Statement &statement, const temp::Map &tempMap);
void dump(const Statements &statements, const temp::Map &tempMap);

/* a op b    ==     not(a notRel(op) b)  */
RelOp notRel(RelOp op);

/* a op b    ==    b commute(op) a       */
RelOp commute(RelOp op);

} // namespace ir

} // namespace tiger
