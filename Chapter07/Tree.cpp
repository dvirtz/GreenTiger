#include "Tree.h"
#include <cassert>
#include <iomanip>

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

void printIndentation(std::ostream &ost, int indentation) {
  ost << std::setw(indentation) << ' ';
}

void printStatement(std::ostream &ost, const Statement &statement,
                    int indent /*= 0*/) {
  using helpers::match;

  printIndentation(ost, indent++);
  match(statement)(
    [&](const Sequence &seq) {
      ost << "SEQ(\n";
      for (const auto &statement : seq.statements) {
        printStatement(ost, statement, indent);
        ost << ",\n";
      }
      ost << ')';
    },
    [&](const temp::Label &label) { ost << "LABEL " << label; },
    [&](const Jump &jump) {
      ost << "JUMP(\n";
      printExpression(ost, jump.exp, indent);
      ost << ')';
    },
    [&](const ConditionalJump &cjump) {
      ost << "CJUMP(" << cjump.op << ",\n";
      printExpression(ost, cjump.left, indent);
      ost << ",\n";
      printExpression(ost, cjump.right, indent);
      printIndentation(ost, indent);
      ost << *cjump.trueDest << ", " << *cjump.falseDest << ')';
    },
    [&](const Move &move) {
      ost << "MOVE(\n";
      printExpression(ost, move.dst, indent);
      ost << ",\n";
      printExpression(ost, move.src, indent);
      ost << ')';
    },
    [&](const ExpressionStatement &exp) {
      ost << "EXP(\n";
      printExpression(ost, exp.exp, indent);
      ost << ')';
    });
}

void printExpression(std::ostream &ost, const Expression &expression,
                     int indent /*= 0*/) {
  using helpers::match;

  printIndentation(ost, indent++);
  match(expression)(
    [&](const BinaryOperation &binOperation) {
      ost << "BINOP(" << binOperation.op << ",\n";
      printExpression(ost, binOperation.left, indent);
      ost << ",\n";
      printExpression(ost, binOperation.right, indent);
      ost << ')';
    },
    [&](const MemoryAccess &memAccess) {
      ost << "MEM(\n";
      printExpression(ost, memAccess.address, indent);
      ost << ')';
    },
    [&](const temp::Register &reg) { ost << "TEMP t" << reg; },
    [&](const ExpressionSequence &seq) {
      ost << "ESEQ(\n";
      printStatement(ost, seq.stm, indent);
      ost << ",\n";
      printExpression(ost, seq.exp, indent);
      ost << ')';
    },
    [&](const temp::Label &label) { ost << "NAME " << label; },
    [&](int i) { ost << "CONST " << i; },
    [&](const Call &call) {
      ost << "CALL(\n";
      printExpression(ost, call.fun, indent++);
      for (const auto &arg : call.args) {
        ost << ",\n";
        printExpression(ost, arg, indent);
      }
      ost << ')';
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
