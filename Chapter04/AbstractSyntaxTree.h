#pragma once
#include "variantMatch.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/support_unused.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <ostream>
#include <string>
#include <vector>

namespace tiger {

namespace ast {
///////////////////////////////////////////////////////////////////////////
//  The AST
///////////////////////////////////////////////////////////////////////////
struct Tagged {
  int id; // Used to annotate the AST with the iterator position.
          // This id is used as a key to a map<int, Iterator>
          // (not really part of the AST.)
};

enum class Operation {
  PLUS,
  MINUS,
  TIMES,
  DIVIDE,
  EQUAL,
  NOT_EQUAL,
  LESS_THEN,
  GREATER_THEN,
  LESS_EQUAL,
  GREATER_EQUAL,
  AND,
  OR
};

struct Identifier : Tagged {
  std::string name;
  // https://stackoverflow.com/a/19824426/621176
  boost::spirit::unused_type dummy;
};

struct IntExpression {
  uint32_t i;
};

struct StringExpression : Tagged {
  std::string s;
};

struct NilExpression {};
struct VarExpression;
struct CallExpression;
struct ArithmeticExpression;
struct RecordExpression;
struct AssignExpression;
struct IfExpression;
struct WhileExpression;
struct BreakExpression {};
struct ForExpression;
struct LetExpression;
struct ArrayExpression;
struct ExpressionSequence;

using Expression =
  boost::variant<NilExpression, boost::recursive_wrapper<VarExpression>,
                 IntExpression, StringExpression,
                 boost::recursive_wrapper<CallExpression>,
                 boost::recursive_wrapper<ArithmeticExpression>,
                 boost::recursive_wrapper<RecordExpression>,
                 boost::recursive_wrapper<AssignExpression>,
                 boost::recursive_wrapper<IfExpression>,
                 boost::recursive_wrapper<WhileExpression>, BreakExpression,
                 boost::recursive_wrapper<ForExpression>,
                 boost::recursive_wrapper<LetExpression>,
                 boost::recursive_wrapper<ArrayExpression>,
                 boost::recursive_wrapper<ExpressionSequence>>;

struct VarField {
  Identifier name;
};

struct Subscript {
  Expression exp;
};

using VarRestElement = boost::variant<VarField, Subscript>;

using VarRest = std::vector<VarRestElement>;

struct VarExpression {
  Identifier first;
  VarRest rest;
};

using Expressions = std::vector<Expression>;

struct CallExpression {
  Identifier func;
  Expressions args;
};

struct OperationExpression {
  Operation op;
  Expression exp;
};

using OperationExpressions = std::vector<OperationExpression>;

struct ArithmeticExpression {
  Expression first;
  OperationExpressions rest;
};

struct RecordField {
  Identifier name;
  Expression exp;
};

using RecordFields = std::vector<RecordField>;

struct RecordExpression {
  Identifier type;
  RecordFields fields;
};

struct AssignExpression {
  VarExpression var;
  Expression exp;
};

struct IfExpression {
  Expression test;
  Expression thenExp;
  Expression elseExp;
};

struct WhileExpression {
  Expression test;
  Expression body;
};

struct ForExpression {
  Identifier var;
  Expression lo;
  Expression hi;
  Expression body;
};

struct Field {
  Identifier name;
  Identifier type;
};

using Fields = std::vector<Field>;

struct FunctionDeclaration {
  Identifier name;
  Fields params;
  boost::optional<Identifier> result;
  Expression body;
};

using FunctionDeclarations = std::vector<FunctionDeclaration>;

struct VarDeclaration {
  Identifier name;
  boost::optional<Identifier> type;
  Expression init;
};

struct NameType {
  Identifier name;
};

struct RecordType {
  Fields record;
};

struct ArrayType {
  Identifier array;
};

using Type = boost::variant<NameType, RecordType, ArrayType>;

struct TypeDeclaration {
  Identifier name;
  Type type;
};

using TypeDeclarations = std::vector<TypeDeclaration>;

using Declaration =
  boost::variant<FunctionDeclarations, VarDeclaration, TypeDeclarations>;

using Declarations = std::vector<Declaration>;

struct LetExpression {
  Declarations decs;
  Expressions body;
};

struct ArrayExpression {
  Identifier type;
  Expression size;
  Expression init;
};

struct ExpressionSequence {
  Expressions exps;
};

// print functions for debugging
inline std::ostream &operator<<(std::ostream &out, const Identifier id) {
  return out << id.name;
}

inline std::ostream &operator<<(std::ostream &out, BreakExpression) {
  return out << "break";
}

inline std::ostream &operator<<(std::ostream &out, NilExpression) {
  return out << "nil";
}

inline std::ostream &operator<<(std::ostream &out, Operation op) {
  auto str = [&]() {
    switch (op) {
      case tiger::ast::Operation::PLUS:
        return "+";
      case tiger::ast::Operation::MINUS:
        return "-";
      case tiger::ast::Operation::TIMES:
        return "*";
      case tiger::ast::Operation::DIVIDE:
        return "/";
      case tiger::ast::Operation::EQUAL:
        return "=";
      case tiger::ast::Operation::NOT_EQUAL:
        return "<>";
      case tiger::ast::Operation::LESS_THEN:
        return "<";
      case tiger::ast::Operation::GREATER_THEN:
        return ">";
      case tiger::ast::Operation::LESS_EQUAL:
        return "<=";
      case tiger::ast::Operation::GREATER_EQUAL:
        return ">=";
      case tiger::ast::Operation::AND:
        return "&";
      case tiger::ast::Operation::OR:
        return "|";
      default:
        assert(false && "Unknown operation");
        return "";
    }
  }();
  return out << str;
}

inline void simplifyTree(ast::Expression &ast) {
  using helpers::match;

  auto noop        = [&](auto & /* e */) {};
  auto simplifyVar = [&noop](ast::VarExpression &e) {
    for (auto &ee : e.rest) {
      match(ee)([&](ast::Subscript &s) { simplifyTree(s.exp); }, noop);
    }
  };
  match(ast)([&](ast::VarExpression &e) { simplifyVar(e); },
             [&](ast::CallExpression &e) {
               for (auto &ee : e.args) {
                 simplifyTree(ee);
               }
             },
             [&](ast::ArithmeticExpression &e) {
               simplifyTree(e.first);
               if (e.rest.empty()) {
                 auto tmp = std::move(e.first);
                 ast      = tmp;
               } else {
                 for (auto &ee : e.rest) {
                   simplifyTree(ee.exp);
                 }
               }
             },
             [&](ast::RecordExpression &e) {
               for (auto &ee : e.fields) {
                 simplifyTree(ee.exp);
               }
             },
             [&](ast::AssignExpression &e) {
               simplifyVar(e.var);
               simplifyTree(e.exp);
             },
             [&](ast::IfExpression &e) {
               simplifyTree(e.test);
               simplifyTree(e.thenExp);
               simplifyTree(e.elseExp);
             },
             [&](ast::WhileExpression &e) {
               simplifyTree(e.test);
               simplifyTree(e.body);
             },
             [&](ast::ForExpression &e) {
               simplifyTree(e.lo);
               simplifyTree(e.hi);
               simplifyTree(e.body);
             },
             [&](ast::LetExpression &e) {
               for (auto &dec : e.decs) {
                 match(dec)(
                   [&](FunctionDeclarations &funcs) {
                     for (auto &func : funcs) {
                       simplifyTree(func.body);
                     }
                   },
                   [&](VarDeclaration &var) { simplifyTree(var.init); }, noop);
               }
               for (auto &ee : e.body) {
                 simplifyTree(ee);
               }
             },
             [&](ast::ArrayExpression &e) {
               simplifyTree(e.size);
               simplifyTree(e.init);
             },
             [&](ast::ExpressionSequence &e) {
               for (auto &ee : e.exps) {
                 simplifyTree(ee);
               }
             },
             noop);
}

} // namespace ast

} // namespace tiger

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::Identifier,
                          (std::string, name)(boost::spirit::unused_type,
                                              dummy))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::NameType, (tiger::ast::Identifier, name))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::Field,
                          (tiger::ast::Identifier, name)(tiger::ast::Identifier,
                                                         type))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::RecordType, (tiger::ast::Fields, record))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::ArrayType,
                          (tiger::ast::Identifier, array))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::TypeDeclaration,
                          (tiger::ast::Identifier, name)(tiger::ast::Type,
                                                         type))

BOOST_FUSION_ADAPT_STRUCT(
  tiger::ast::FunctionDeclaration,
  (tiger::ast::Identifier,
   name)(tiger::ast::Fields, params)(boost::optional<tiger::ast::Identifier>,
                                     result)(tiger::ast::Expression, body))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::VarDeclaration,
                          (tiger::ast::Identifier,
                           name)(boost::optional<tiger::ast::Identifier>,
                                 type)(tiger::ast::Expression, init))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::VarExpression,
                          (tiger::ast::Identifier, first)(tiger::ast::VarRest,
                                                          rest))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::Subscript, (tiger::ast::Expression, exp))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::VarField, (tiger::ast::Identifier, name))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::IntExpression, (uint32_t, i))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::StringExpression, (std::string, s))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::CallExpression,
                          (tiger::ast::Identifier,
                           func)(tiger::ast::Expressions, args))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::OperationExpression,
                          (tiger::ast::Operation, op)(tiger::ast::Expression,
                                                      exp))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::ArithmeticExpression,
                          (tiger::ast::Expression,
                           first)(tiger::ast::OperationExpressions, rest))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::RecordField,
                          (tiger::ast::Identifier, name)(tiger::ast::Expression,
                                                         exp))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::RecordExpression,
                          (tiger::ast::Identifier,
                           type)(tiger::ast::RecordFields, fields))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::AssignExpression,
                          (tiger::ast::VarExpression,
                           var)(tiger::ast::Expression, exp))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::IfExpression,
                          (tiger::ast::Expression,
                           test)(tiger::ast::Expression,
                                 thenExp)(tiger::ast::Expression, elseExp))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::WhileExpression,
                          (tiger::ast::Expression, test)(tiger::ast::Expression,
                                                         body))

// BOOST_FUSION_ADAPT_STRUCT_BASE(
//   (0),
//   (0)(tiger::ast::BreakExpression),
//   struct_tag,
//   0,
//   (0,0),
//   BOOST_FUSION_ADAPT_STRUCT_C
// )

// BOOST_FUSION_ADAPT_STRUCT(
//   tiger::ast::BreakExpression
// )

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::ForExpression,
                          (tiger::ast::Identifier,
                           var)(tiger::ast::Expression,
                                lo)(tiger::ast::Expression,
                                    hi)(tiger::ast::Expression, body))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::LetExpression,
                          (tiger::ast::Declarations,
                           decs)(tiger::ast::Expressions, body))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::ArrayExpression,
                          (tiger::ast::Identifier,
                           type)(tiger::ast::Expression,
                                 size)(tiger::ast::Expression, init))

BOOST_FUSION_ADAPT_STRUCT(tiger::ast::ExpressionSequence,
                          (tiger::ast::Expressions, exps))
