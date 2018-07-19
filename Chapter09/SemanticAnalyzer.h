#pragma once
#include "AbstractSyntaxTree.h"
#include "Translator.h"
#include "Types.h"
#include <boost/optional.hpp>
#include <functional>
#include <unordered_map>
#include <vector>

namespace tiger {

struct CompiledExpression {
  NamedType m_type;
  translator::Expression m_translated;
};

class SemanticAnalyzer {
public:
  using result_type = CompiledExpression;

  template <typename ErrorHandler, typename Annotation>
  SemanticAnalyzer(ErrorHandler &errorHandler, Annotation &annotation,
                   temp::Map &tempMap,
                   frame::CallingConvention &callingConvention)
      : m_errorHandler{[&errorHandler, &annotation](size_t id,
                                                    const std::string &what) {
          errorHandler("Error", what, annotation.iteratorFromId(id));
        }},
        m_translator{tempMap, callingConvention}, m_tempMap{tempMap} {
    m_environments.push_back(defaultEnvironment());
  }

  FragmentList compile(const ast::Expression &ast);

private:
  result_type compileExpression(const ast::Expression &ast);
  result_type compileExpression(const ast::NilExpression &exp);
  result_type compileExpression(const ast::VarExpression &exp);
  result_type compileExpression(const ast::IntExpression &exp);
  result_type compileExpression(const ast::StringExpression &exp);
  result_type compileExpression(const ast::CallExpression &exp);
  result_type compileExpression(const ast::ArithmeticExpression &exp);
  result_type compileExpression(const ast::RecordExpression &exp);
  result_type compileExpression(const ast::AssignExpression &exp);
  result_type compileExpression(const ast::IfExpression &exp);
  result_type compileExpression(const ast::WhileExpression &exp);
  result_type compileExpression(const ast::BreakExpression &exp);
  result_type compileExpression(const ast::ForExpression &exp);
  result_type compileExpression(const ast::LetExpression &exp);
  result_type compileExpression(const ast::ArrayExpression &exp);
  result_type compileExpression(const ast::ExpressionSequence &exp);

  template <typename T> bool hasType(const NamedType &type) const {
    return boost::get<T>(&type.m_type) != nullptr;
  }

  template <typename T> bool hasType(const CompiledExpression &exp) const {
    return hasType<T>(exp.m_type);
  }

  bool equalTypes(const NamedType &lhs, const NamedType &rhs) const {
    // nil can be converted to a record
    return lhs.m_name == rhs.m_name ||
           (hasType<NilType>(lhs) && hasType<RecordType>(rhs)) ||
           (hasType<RecordType>(lhs) && hasType<NilType>(rhs));
  }

  bool equalTypes(const CompiledExpression &lhs,
                  const CompiledExpression &rhs) const {
    return equalTypes(lhs.m_type, rhs.m_type);
  }

  size_t id(const ast::Expression &expression) const;

  struct VariableType {
    NamedType m_type;
    translator::VariableAccess m_access;
  };

  struct FunctionType {
    NamedType m_resultType;
    std::vector<NamedType> m_parameterTypes;
    temp::Label m_label;
    translator::Level m_declerationLevel;
    translator::Level m_bodyLevel;
  };

  using ValueType = boost::variant<VariableType, FunctionType>;

  using OptionalValue = boost::optional<ValueType>;

  OptionalValue findValue(const std::string &name) const;

  using OptionalType = boost::optional<NamedType>;

  OptionalType findType(std::string name) const;

  using CompiledDeclaration = translator::Expression;

  CompiledDeclaration addToEnv(const ast::Declaration &dec);
  CompiledDeclaration addToEnv(const ast::FunctionDeclarations &decs);
  CompiledDeclaration addToEnv(const ast::VarDeclaration &dec);
  CompiledDeclaration addToEnv(const ast::TypeDeclarations &decs);

  using TypeMap = std::unordered_map<std::string, NamedType>;

  void addToEnv(const TypeMap::key_type &name,
                const TypeMap::mapped_type &type);

  using ValueMap = std::unordered_map<std::string, ValueType>;

  void addToEnv(const ValueMap::key_type &name,
                const ValueMap::mapped_type &value);

  ir::BinOp toBinOp(ast::Operation op);

  ir::RelOp toRelOp(ast::Operation op);

  std::function<void(size_t, const std::string &)> m_errorHandler;

  translator::Translator m_translator;
  temp::Map &m_tempMap;

  struct Environment {
    TypeMap m_typeMap;
    ValueMap m_valueMap;
  };

  Environment defaultEnvironment();

  std::vector<Environment> m_environments;

  static const NamedType s_intType;
  static const NamedType s_stringType;
  static const NamedType s_voidType;
  static const NamedType s_nilType;

  std::vector<temp::Label> m_breakTargets;
  std::vector<translator::Level> m_functionLevels;
};

} // namespace tiger
