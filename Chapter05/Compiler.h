#pragma once
#include "AbstractSyntaxTree.h"
#include "Types.h"
#include <functional>
#include <boost/optional.hpp>
#include <unordered_map>
#include <vector>

namespace tiger
{

struct CompiledExpression
{
  NamedType m_type;
};

class Compiler
{
public:
  using result_type = boost::optional<CompiledExpression>;

  template<typename ErrorHandler, typename Annotation>
  Compiler(ErrorHandler& errorHandler, Annotation& annotation)
    : m_errorHandler([&errorHandler, &annotation](size_t id, const std::string& what)
  {
    errorHandler("Error", what, annotation.iteratorFromId(id));
    return ErrorResult{};
  })
  {
    m_environments.push_back(defaultEnvironment());
  }

  result_type compile(const ast::Expression& ast);

  result_type operator()(const ast::NilExpression& exp);
  result_type operator()(const ast::VarExpression& exp);
  result_type operator()(const ast::IntExpression& exp);
  result_type operator()(const ast::StringExpression& exp);
  result_type operator()(const ast::CallExpression& exp);
  result_type operator()(const ast::ArithmeticExpression& exp);
  result_type operator()(const ast::RecordExpression& exp);
  result_type operator()(const ast::AssignExpression& exp);
  result_type operator()(const ast::IfExpression& exp);
  result_type operator()(const ast::WhileExpression& exp);
  result_type operator()(const ast::BreakExpression& exp);
  result_type operator()(const ast::ForExpression& exp);
  result_type operator()(const ast::LetExpression& exp);
  result_type operator()(const ast::ArrayExpression& exp);
  result_type operator()(const ast::ExpressionSequence& exp);

private:
  template<typename T>
  bool hasType(const CompiledExpression& exp) const
  {
    return boost::get<T>(&exp.m_type.m_type) != nullptr;
  }

  size_t id(const ast::Expression& expression) const;

  // dummy struct which can be converted to either empty result_type or false
  struct ErrorResult {
    template<typename T>
    operator boost::optional<T>() const
    {
      return {};
    }

    operator bool() const
    {
      return false;
    }
  };

  std::function<ErrorResult(size_t, const std::string&)> m_errorHandler;

  using TypeMap = std::unordered_map<std::string, NamedType>;

  struct VariableType
  {
    NamedType m_type;
  };

  struct FunctionType
  {
    NamedType              m_resultType;
    std::vector<NamedType> m_parameterTypes;
  };

  using ValueType = boost::variant<VariableType, FunctionType>;

  using ValueMap = std::unordered_map<std::string, ValueType>;

  struct Environment
  {
    TypeMap   m_typeMap;
    ValueMap  m_valueMap;
  };

  Environment defaultEnvironment();

  using OptionalValue = boost::optional<ValueType>;

  OptionalValue findValue(const std::string& name) const;

  using OptionalType = boost::optional<NamedType>;

  OptionalType findType(std::string name) const;

  bool addToEnv(const ast::Declaration& dec);
  bool addToEnv(const ast::FunctionDeclarations& decs);
  bool addToEnv(const ast::VarDeclaration& dec);
  bool addToEnv(const ast::TypeDeclarations& decs);

  void addToEnv(const TypeMap::key_type& name, const TypeMap::mapped_type& type);
  void addToEnv(const ValueMap::key_type& name, const ValueMap::mapped_type& value);

  bool m_withinLoop = false;

  std::vector<Environment> m_environments;

  static const NamedType s_intType;
  static const NamedType s_stringType;
  static const NamedType s_voidType;
  static const NamedType s_nilType;
};

} // namespace tiger