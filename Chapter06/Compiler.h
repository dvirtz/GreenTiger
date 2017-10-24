#pragma once
#include "AbstractSyntaxTree.h"
#include "Types.h"
#include "Translator.h"
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
  Compiler(ErrorHandler& errorHandler, Annotation& annotation, TempMap& tempMap, Translator& translator)
    : m_errorHandler([&errorHandler, &annotation](size_t id, const std::string& what)
  {
    errorHandler("Error", what, annotation.iteratorFromId(id));
    return ErrorResult{};
  }),
    m_tempMap(tempMap),
    m_translator(translator)
  {
    m_environments.push_back(defaultEnvironment());
  }

  result_type compile(const ast::Expression& ast);
  
private:

  result_type compile(Translator::Level level, const ast::Expression& ast);
  result_type compile(Translator::Level level, const ast::NilExpression& exp);
  result_type compile(Translator::Level level, const ast::VarExpression& exp);
  result_type compile(Translator::Level level, const ast::IntExpression& exp);
  result_type compile(Translator::Level level, const ast::StringExpression& exp);
  result_type compile(Translator::Level level, const ast::CallExpression& exp);
  result_type compile(Translator::Level level, const ast::ArithmeticExpression& exp);
  result_type compile(Translator::Level level, const ast::RecordExpression& exp);
  result_type compile(Translator::Level level, const ast::AssignExpression& exp);
  result_type compile(Translator::Level level, const ast::IfExpression& exp);
  result_type compile(Translator::Level level, const ast::WhileExpression& exp);
  result_type compile(Translator::Level level, const ast::BreakExpression& exp);
  result_type compile(Translator::Level level, const ast::ForExpression& exp);
  result_type compile(Translator::Level level, const ast::LetExpression& exp);
  result_type compile(Translator::Level level, const ast::ArrayExpression& exp);
  result_type compile(Translator::Level level, const ast::ExpressionSequence& exp);

  template<typename T>
  bool hasType(const NamedType& type) const
  {
    return boost::get<T>(&type.m_type) != nullptr;
  }

  template<typename T>
  bool hasType(const CompiledExpression& exp) const
  {
    return hasType<T>(exp.m_type);
  }

  bool equalTypes(const NamedType& lhs, const NamedType& rhs) const
  {
    // nil can be converted to a record
    return lhs.m_name == rhs.m_name
      || (hasType<NilType>(lhs) && hasType<RecordType>(rhs))
      || (hasType<RecordType>(lhs) && hasType<NilType>(rhs))
      ;
  }

  bool equalTypes(const CompiledExpression& lhs, const CompiledExpression& rhs) const
  {
    return equalTypes(lhs.m_type, rhs.m_type);
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

  TempMap&    m_tempMap;
  Translator&   m_translator;

  using TypeMap = std::unordered_map<std::string, NamedType>;

  struct VariableType
  {
    NamedType               m_type;
    Translator::Level       m_level;
    VariableAccess          m_access;
  };

  struct FunctionType
  {
    NamedType               m_resultType;
    std::vector<NamedType>  m_parameterTypes;
    Label                   m_label;
    Translator::Level       m_level;
  };

  using ValueType = boost::variant<VariableType, FunctionType>;

  using ValueMap = std::unordered_map<std::string, ValueType>;

  struct Environment
  {
    TypeMap                 m_typeMap;
    ValueMap                m_valueMap;
  };

  Environment defaultEnvironment();

  using OptionalValue = boost::optional<ValueType>;

  OptionalValue findValue(const std::string& name) const;

  using OptionalType = boost::optional<NamedType>;

  OptionalType findType(std::string name) const;

  bool addToEnv(Translator::Level level, const ast::Declaration& dec);
  bool addToEnv(Translator::Level level, const ast::FunctionDeclarations& decs);
  bool addToEnv(Translator::Level level, const ast::VarDeclaration& dec);
  bool addToEnv(Translator::Level level, const ast::TypeDeclarations& decs);

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