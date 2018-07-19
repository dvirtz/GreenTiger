#include "Tree.h"
#include <boost/fusion/include/adapt_struct.hpp>

#ifdef _MSC_VER
#define PUSH_AND_DISABLE_MSVC_WARNING(WarningNumber)                           \
  __pragma(warning(push)) __pragma(warning(disable : WarningNumber))

#define POP_MSVC_WARNING __pragma(warning(pop))
#else
#define PUSH_AND_DISABLE_MSVC_WARNING(WarningNumber)
#define POP_MSVC_WARNING
#endif

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::Sequence,
  (tiger::ir::Statements, statements)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::Jump,
  (tiger::ir::Expression, exp)
  // ignore jumps
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::ConditionalJump,
  (tiger::ir::RelOp, op)
  (tiger::ir::Expression, left)
  (tiger::ir::Expression, right)
  (std::shared_ptr<tiger::temp::Label>, trueDest)
  // ignore falseDest
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::Move,
  (tiger::ir::Expression, dst)
  (tiger::ir::Expression, src)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::ExpressionStatement,
  (tiger::ir::Expression, exp)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::BinaryOperation,
  (tiger::ir::BinOp, op)
  (tiger::ir::Expression, left)
  (tiger::ir::Expression, right)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::MemoryAccess,
  (tiger::ir::Expression, address)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::ExpressionSequence,
  (tiger::ir::Expression, exp)
  (tiger::ir::Statement, stm)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::Call,
  (tiger::ir::Expression, fun)
  (std::vector<tiger::ir::Expression>, args)
)

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ir::Placeholder,
  (boost::optional<tiger::ir::Expression>, exp)
)
