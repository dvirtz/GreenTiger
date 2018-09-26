#include "Tree.h"
#include <cassert>
#include <iomanip>
#include <iostream>

namespace tiger {
namespace ir {

std::ostream &operator<<(std::ostream &ost, BinOp binOp) {
  switch (binOp) {
    case BinOp::PLUS:
      return ost << "PLUS";
    case BinOp::MINUS:
      return ost << "MINUS";
    case BinOp::MUL:
      return ost << "TIMES";
    case BinOp::DIV:
      return ost << "DIVIDE";
    case BinOp::AND:
      return ost << "AND";
    case BinOp::OR:
      return ost << "OR";
    case BinOp::LSHIFT:
      return ost << "LSHIFT";
    case BinOp::RSHIFT:
      return ost << "RSHIFT";
    case BinOp::ARSHIFT:
      return ost << "ARSHIFT";
    case BinOp::XOR:
      return ost << "XOR";
    default:
      assert(false && "Unknown BinOp");
      return ost;
  }
}

std::ostream &operator<<(std::ostream &ost, RelOp relOp) {
  switch (relOp) {
    case RelOp::EQ:
      return ost << "EQ";
    case RelOp::NE:
      return ost << "NE";
    case RelOp::LT:
      return ost << "LT";
    case RelOp::GT:
      return ost << "GT";
    case RelOp::LE:
      return ost << "LE";
    case RelOp::GE:
      return ost << "GE";
    case RelOp::ULT:
      return ost << "ULT";
    case RelOp::ULE:
      return ost << "ULE";
    case RelOp::UGT:
      return ost << "UGT";
    case RelOp::UGE:
      return ost << "UGE";
    default:
      assert(false && "Unknown RelOp");
      return ost;
  }
}

std::ostream &operator<<(std::ostream &ost, const Expression &expression) {
  printExpression(ost, expression);
  return ost;
}

std::ostream &operator<<(std::ostream &ost, const Statement &statement) {
  printStatement(ost, statement);
  return ost;
}

std::ostream &operator<<(std::ostream &ost, const Statements &statements) {
  printStatements(ost, statements);
  return ost;
}

void printStatements(std::ostream &ost, const Statements &statements,
                     const int indent /*= 0*/) {
  for (const auto &statement : statements) {
    printStatement(ost, statement, indent);
    ost << ",\n";
  }
}

void dump(const Expression &expression) { std::cout << expression << '\n'; }

void dump(const Statement &statement) { std::cout << statement << '\n'; }

void dump(const Statements &statements) { std::cout << statements << '\n'; }

void printIndentation(std::ostream &ost, int indentation,
                      bool closing /*= false*/) {
  if (closing)
    ost << '\n';
  if (indentation)
    ost << std::setw(indentation) << ' ';
  if (closing)
    ost << ')';
}

void printStatement(std::ostream &ost, const Statement &statement,
                    const int indent /*= 0*/) {
  using helpers::match;

  printIndentation(ost, indent);
  match(statement)(
    [&](const Sequence &seq) {
      ost << "SEQ(\n";
      printStatements(ost, seq.statements, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const temp::Label &label) { ost << "LABEL " << label; },
    [&](const Jump &jump) {
      ost << "JUMP(\n";
      printExpression(ost, jump.exp, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const ConditionalJump &cjump) {
      ost << "CJUMP(" << cjump.op << ",\n";
      printExpression(ost, cjump.left, indent + 1);
      ost << ",\n";
      printExpression(ost, cjump.right, indent + 1);
      ost << ",\n";
      printIndentation(ost, indent);
      auto ptrStr = [](const std::shared_ptr<temp::Label> &p) {
        using namespace std::string_literals;
        return p ? p->get() : "NULL"s;
      };
      ost << ptrStr(cjump.trueDest) << ", " << ptrStr(cjump.falseDest);
      printIndentation(ost, indent, true);
    },
    [&](const Move &move) {
      ost << "MOVE(\n";
      printExpression(ost, move.dst, indent + 1);
      ost << ",\n";
      printExpression(ost, move.src, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const ExpressionStatement &exp) {
      ost << "EXP(\n";
      printExpression(ost, exp.exp, indent + 1);
      printIndentation(ost, indent, true);
    });
}

void printExpression(std::ostream &ost, const Expression &expression,
                     const int indent /*= 0*/) {
  using helpers::match;

  printIndentation(ost, indent);
  match(expression)(
    [&](const BinaryOperation &binOperation) {
      ost << "BINOP(" << binOperation.op << ",\n";
      printExpression(ost, binOperation.left, indent + 1);
      ost << ",\n";
      printExpression(ost, binOperation.right, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const MemoryAccess &memAccess) {
      ost << "MEM(\n";
      printExpression(ost, memAccess.address, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const temp::Register &reg) { ost << "TEMP t" << reg; },
    [&](const ExpressionSequence &seq) {
      ost << "ESEQ(\n";
      printStatement(ost, seq.stm, indent + 1);
      ost << ",\n";
      printExpression(ost, seq.exp, indent + 1);
      printIndentation(ost, indent, true);
    },
    [&](const temp::Label &label) { ost << "NAME " << label; },
    [&](int i) { ost << "CONST " << i; },
    [&](const Call &call) {
      ost << "CALL(\n";
      printExpression(ost, call.fun, indent + 1);
      for (const auto &arg : call.args) {
        ost << ",\n";
        printExpression(ost, arg, indent + 2);
      }
      printIndentation(ost, indent, true);
    });
}

RelOp notRel(RelOp op) {
  switch (op) {
    default:
      assert(false && "Unknown operation");
    case RelOp::EQ:
      return RelOp::NE;
    case RelOp::NE:
      return RelOp::EQ;
    case RelOp::LT:
      return RelOp::GE;
    case RelOp::GE:
      return RelOp::LT;
    case RelOp::GT:
      return RelOp::LE;
    case RelOp::LE:
      return RelOp::GT;
    case RelOp::ULT:
      return RelOp::UGE;
    case RelOp::UGE:
      return RelOp::ULT;
    case RelOp::ULE:
      return RelOp::UGT;
    case RelOp::UGT:
      return RelOp::ULE;
  }
}

RelOp commute(RelOp op) {
  switch (op) {
    default:
      assert(false && "Unknown operation");
    case RelOp::EQ:
      return RelOp::EQ;
    case RelOp::NE:
      return RelOp::NE;
    case RelOp::LT:
      return RelOp::GT;
    case RelOp::GE:
      return RelOp::LE;
    case RelOp::GT:
      return RelOp::LT;
    case RelOp::LE:
      return RelOp::GE;
    case RelOp::ULT:
      return RelOp::UGT;
    case RelOp::UGE:
      return RelOp::ULE;
    case RelOp::ULE:
      return RelOp::UGE;
    case RelOp::UGT:
      return RelOp::ULT;
  }
}

} // namespace ir
} // namespace tiger
