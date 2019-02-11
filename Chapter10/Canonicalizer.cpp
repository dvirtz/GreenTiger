#include "Canonicalizer.h"
#include "CallingConvention.h"
#include "variantMatch.h"
#include <numeric>

namespace tiger {

using helpers::match;

Canonicalizer::Canonicalizer(temp::Map &tempMap) : m_tempMap{tempMap} {}

ir::Statements Canonicalizer::canonicalize(ir::Statement &&statement) {
  // add jump to end
  auto end = m_tempMap.newLabel();
  ir::Sequence seq{{std::move(statement), ir::Jump{end}}};
  auto res = traceSchedule(makeBasicBlocks(linearize(std::move(seq))));
  res.emplace_back(end);
  return res;
}

tiger::ir::Statements
  Canonicalizer::linearize(ir::Statement &&statement) const {
  auto reordered = reorderStatement(statement);
  return linearizeHelper(reordered);
}

Canonicalizer::BasicBlocks
  Canonicalizer::makeBasicBlocks(ir::Statements &&statements) const {
  //   The sequence is scanned from beginning to end; whenever
  //   a LABEL is found, a new block is started(and the previous block is
  //   ended); Whenever a JUMP or CJUMP is found, a block is ended(and the next
  //   block is started).If this leaves any block not ending with a JUMP or
  //   CJUMP, then a JUMP to the next block�s label is appended to the block. If
  //   any block has been left without a LABEL at the beginning, a new label is
  //   invented and stuck there.

  BasicBlocks res;

  auto insertNewBlock = [&statements, &res, this](ir::Statements::iterator to) {
    auto from = statements.begin();
    if (to == from) {
      return std::next(to);
    }

    ir::Statements newBlock;
    newBlock.reserve(std::distance(from, to) + 1);
    // make sure block begins with a label
    match(statements.front())(
      [](temp::Label & /* label */) {
        // block already starts with a label
      },
      [&newBlock, this](auto & /*default*/) {
        newBlock.emplace_back(m_tempMap.newLabel());
      });
    std::move(from, to, std::back_inserter(newBlock));
    // make sure block ends with a jump
    match(newBlock.back())([](const ir::Jump &) {},
                           [](const ir::ConditionalJump &) {},
                           [&](const auto &) {
                             auto label = boost::get<temp::Label>(*to);
                             newBlock.emplace_back(ir::Jump{label});
                           });
    res.emplace_back(std::move(newBlock));
    return statements.erase(from, to);
  };

  for (auto it = statements.begin(); it != statements.end();) {
    match (*it)(
      [&](temp::Label & /* label */) { it = insertNewBlock(it); },
      [&](ir::Jump & /* jump */) { it = insertNewBlock(std::next(it)); },
      [&](ir::ConditionalJump & /* jump */) {
        it = insertNewBlock(std::next(it));
      },
      [&](auto & /*default*/) { ++it; });
  }

  // the last statement was a jump to end
  assert(statements.empty());

  return res;
}

ir::Statements Canonicalizer::traceSchedule(BasicBlocks &&blocks) const {
  ir::Statements res;
  while (!blocks.empty()) {
    auto addToTrace = [&res, &blocks](auto it) {
      std::move(it->begin(), it->end(), std::back_inserter(res));
      return blocks.erase(it);
    };

    auto findSuccessor = [&blocks](const auto &pLabel) {
      if (!pLabel) {
        return blocks.end();
      }

      // find block which starts with label
      return std::find_if(
        blocks.begin(), blocks.end(), [&pLabel](const ir::Statements &b) {
          return match(b.front())(
            [&pLabel](const temp::Label &l) { return l == *pLabel; },
            [](const auto & /*default*/) { return false; });
        });
    };

    for (auto it = blocks.begin(); it != blocks.end();) {
      // add next block to current trace
      it = addToTrace(it);

      match(res.back())(
        [&](ir::Jump &jump) {
          it = findSuccessor(&jump.jumps.front());
          if (it != blocks.end()) {
            // if this is the only successor, remove the jump and merge the
            // two blocks
            if (jump.jumps.size() == 1) {
              res.pop_back();
            }
          };
        },
        [&](ir::ConditionalJump &cJump) {
          it = findSuccessor(cJump.falseDest);
          if (it == blocks.end()) {
            // Any CJUMP immediately followed by its false label we let alone
            it = findSuccessor(cJump.trueDest);
            if (it != blocks.end()) {
              // For any CJUMP followed by its true label, we switch the true
              // and false labels and negate the condition
              cJump.op = ir::notRel(cJump.op);
              std::swap(cJump.falseDest, cJump.trueDest);
            } else {
              // For any CJUMP(cond, a, b, t, f) followed by neither label, we
              // invent a new false label f' and rewrite the single CJUMP
              // statement as three statements, just to achieve the condition
              // that the CJUMP is followed by its false label
              // CJUMP(cond, a, b, t, f') LABEL f' JUMP(NAME f)
              auto newLabel =
                std::make_shared<temp::Label>(m_tempMap.newLabel());
              newLabel.swap(cJump.falseDest);
              res.emplace_back(*cJump.falseDest);
              if (newLabel) {
                res.emplace_back(ir::Jump(*newLabel));
              }
            }
          }
        },
        [](auto & /*default*/) {
          assert(false && "basic block should end with a jump");
        });
    }
  }

  return res;
}

bool Canonicalizer::commutes(const ir::Statement &stm,
                             const ir::Expression &exp) const {
  return isConst(exp) || isNop(stm);
}

template <typename T, typename... Ts>
std::enable_if_t<!helpers::is_iterator<T>::value, ir::Statement>
  Canonicalizer::reorder(T &t, Ts &... ts) const {
  std::initializer_list<std::reference_wrapper<ir::Expression>> list{t, ts...};
  return reorder(list.begin(), list.end());
}

template <typename Iterator, typename>
ir::Statement Canonicalizer::reorder(Iterator first, Iterator last) const {
  if (first == last) {
    return {};
  }

  ir::Expression &exp = *first;
  return match(exp)(
    [&](ir::Call &call) {
      // move call result to a temporary, so that RV register can be reused
      // later
      auto t = m_tempMap.newTemp();
      exp    = ir::ExpressionSequence{ir::Move{call, t}, t};
      return reorder(first, last);
    },
    [&](auto & /*default*/) -> ir::Statement {
      auto reorderedExpression = this->reorderExpression(exp);
      auto reorderedTail       = this->reorder(std::next(first), last);
      if (this->commutes(reorderedTail, reorderedExpression.m_exp)) {
        // swap location of reorderedTail and reorderedExpression.second
        exp = reorderedExpression.m_exp;
        return this->sequence(reorderedExpression.m_stm, reorderedTail);
      } else {
        // move reorderedExpression.second to a temporary and replace exp with
        // it
        auto t = m_tempMap.newTemp();
        exp    = t;
        return this->sequence(reorderedExpression.m_stm,
                              ir::Move{reorderedExpression.m_exp, t},
                              reorderedTail);
      }
    });
}

ir::Statement Canonicalizer::reorderStatement(ir::Statement &stm) const {
  return match(stm)(
    [this](ir::Sequence &sequence) {
      ir::Statements recurse;
      recurse.reserve(sequence.statements.size());
      std::transform(sequence.statements.begin(), sequence.statements.end(),
                     std::back_inserter(recurse), [this](ir::Statement &stm) {
                       return reorderStatement(stm);
                     });
      return this->sequence(recurse);
    },
    [this, &stm](ir::Jump &jump) { return sequence(reorder(jump.exp), stm); },
    [this, &stm](ir::ConditionalJump &conditionalJump) -> ir::Statement {
      // ignore unused branches
      if (conditionalJump.trueDest->get().empty()) {
        assert(conditionalJump.falseDest->get().empty()
               && "both destinations should be empty or full");
        return ir::Sequence{};
      }

      return sequence(reorder(conditionalJump.left, conditionalJump.right),
                      stm);
    },
    [this, &stm](ir::Move &move) {
      return match(move.dst)(
        [this, &move, &stm](temp::Register & /* t */) {
          return match(move.src)(
            [this, &stm](ir::Call &call) {
              return sequence(reorder(call.args.begin(), call.args.end()), stm);
            },
            [this, &move, &stm](auto & /*default*/) {
              return this->sequence(this->reorder(move.src), stm);
            });
        },
        [this, &stm, &move](ir::MemoryAccess &memoryAccess) {
          return sequence(reorder(memoryAccess.address, move.src), stm);
        },
        [this, &stm, &move](ir::ExpressionSequence &eSeq) {
          auto s   = eSeq.stm;
          move.dst = eSeq.exp;
          ir::Statement seq{sequence(s, stm)};
          return reorderStatement(seq);
        },
        [](auto & /*default*/) {
          assert(false && "dst should be temp or mem only");
          return ir::Statement{};
        });
    },
    [this, &stm](ir::ExpressionStatement &expressionStatement) {
      return match(expressionStatement.exp)(
        [this, &stm](ir::Call &call) {
          return sequence(reorder(call.args.begin(), call.args.end()), stm);
        },
        [this, &stm, &expressionStatement](auto & /*default*/) {
          return this->sequence(this->reorder(expressionStatement.exp), stm);
        });
    },
    [&stm](auto & /*default*/) { return stm; });
}

Canonicalizer::Reordered
  Canonicalizer::reorderExpression(ir::Expression &exp) const {
  return match(exp)(
    [this, &exp](ir::BinaryOperation &binOperation) {
      return Reordered{reorder(binOperation.left, binOperation.right), exp};
    },
    [this, &exp](ir::MemoryAccess &memAccess) {
      return Reordered{reorder(memAccess.address), exp};
    },
    [this](ir::ExpressionSequence &expSequence) {
      auto recurse = reorderExpression(expSequence.exp);
      return Reordered{
        sequence(reorderStatement(expSequence.stm), recurse.m_stm),
        recurse.m_exp};
    },
    [this, &exp](ir::Call &call) {
      return Reordered{reorder(call.args.begin(), call.args.end()), exp};
    },
    [&exp](auto & /*default*/) {
      return Reordered{ir::Statement{}, exp};
    });
}

bool Canonicalizer::isConst(const ir::Expression &expression) const {
  return match(expression)([](int) { return true; },
                           [](const temp::Label &) { return true; },
                           [](const auto & /*default*/) { return false; });
}

bool Canonicalizer::hasNoSideEffects(const ir::Expression &expression) const {
  return match(expression)([](int) { return true; },
                           [](const temp::Label &) { return true; },
                           [](const temp::Register &) { return true; },
                           [](const auto & /*default*/) { return false; });
}

bool Canonicalizer::isNop(const ir::Statement &stm) const {
  return match(stm)(
    [this](const ir::ExpressionStatement &expStatement) {
      return hasNoSideEffects(expStatement.exp);
    },
    [](const ir::Sequence &sequence) { return sequence.statements.empty(); },
    [](const auto & /*default*/) { return false; });
}

template <typename Sequence, typename>
ir::Statement Canonicalizer::sequence(const Sequence &statements) const {
  ir::Sequence res;
  res.statements.reserve(statements.size());
  std::remove_copy_if(statements.begin(), statements.end(),
                      std::back_inserter(res.statements),
                      [this](const ir::Statement &stm) { return isNop(stm); });
  return res;
}

template <typename Statement, typename... Statements>
std::enable_if_t<!helpers::is_sequence<Statement>::value, ir::Statement>
  Canonicalizer::sequence(const Statement &statement,
                          const Statements &... statements) const {
  return sequence(std::initializer_list<Statement>{statement, statements...});
}

ir::Statements Canonicalizer::linearizeHelper(ir::Statement &statement) const {
  return match(statement)(
    [this](ir::Sequence &sequence) {
      return std::accumulate(
        sequence.statements.begin(), sequence.statements.end(),
        ir::Statements{},
        [this](const ir::Statements &current, ir::Statement &next) {
          return concatenate(current, linearizeHelper(next));
        });
    },
    [](auto &stm) { return ir::Statements{stm}; });
}

ir::Statements Canonicalizer::concatenate(const ir::Statements &left,
                                          const ir::Statements &right) const {
  ir::Statements res;
  res.insert(res.end(), left.begin(), left.end());
  res.insert(res.end(), right.begin(), right.end());
  return res;
}

} // namespace tiger
