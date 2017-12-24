#pragma once
#include "TempMap.h"
#include "variantMatch.h"
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

using Statement = boost::variant<boost::recursive_wrapper<Sequence>,
                                 temp::Label, boost::recursive_wrapper<Jump>,
                                 boost::recursive_wrapper<ConditionalJump>,
                                 boost::recursive_wrapper<Move>,
                                 boost::recursive_wrapper<ExpressionStatement>>;

struct BinaryOperation;
struct ExpressionSequence;
struct Call;
struct MemoryAccess;

using Expression = boost::variant<int, temp::Label, temp::Register,
                                  boost::recursive_wrapper<BinaryOperation>,
                                  boost::recursive_wrapper<MemoryAccess>,
                                  boost::recursive_wrapper<ExpressionSequence>,
                                  boost::recursive_wrapper<Call>>;

struct Sequence {
  std::vector<Statement> statements;

  // cannot make the initializer_list ctor default because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60437
  Sequence() = default;
  Sequence(std::initializer_list<Statement> statements)
      : statements(statements) {}
};

struct Jump {
  Expression exp;
  std::vector<temp::Label> jumps;
};

struct ConditionalJump {
  RelOp op;
  Expression left, right;
  // using shared_ptr as boost::variant 1.65 still doesn't handle move only
  // types correctly see https://svn.boost.org/trac10/ticket/6971
  std::shared_ptr<temp::Label> trueDest, falseDest;
};

struct Move {
  Expression dst, src;
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

void printIndentation(std::ostream &ost, int indentation);

void printStatement(std::ostream &ost, const Statement &statement,
                    int indent = 0);

std::ostream &operator<<(std::ostream &ost, const Statement &statement);

void printExpression(std::ostream &ost, const Expression &expression,
                     int indent = 0);

std::ostream &operator<<(std::ostream &ost, const Expression &expression);

/* a op b    ==     not(a notRel(op) b)  */
RelOp notRel(RelOp op);

/* a op b    ==    b commute(op) a       */
RelOp commute(RelOp op);

} // namespace ir

} // namespace tiger
