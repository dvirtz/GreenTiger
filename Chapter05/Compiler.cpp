#include "Compiler.h"
#include "variantMatch.h"

namespace tiger
{
const NamedType Compiler::s_intType{ "int", IntType{} };
const NamedType Compiler::s_stringType{ "string", StringType{} };
const NamedType Compiler::s_voidType{ "void", VoidType{} };
const NamedType Compiler::s_nilType{ "nil", NilType{} };

Compiler::result_type Compiler::compile(const ast::Expression& ast)
{
  return boost::apply_visitor(*this, ast);
}

Compiler::result_type Compiler::operator()(const ast::NilExpression & /*exp*/)
{
  return CompiledExpression{ s_nilType };
}

Compiler::result_type Compiler::operator()(const ast::VarExpression & exp)
{
  std::string name = exp.first;
  auto val = findValue(name);
  if (!val) {
    return m_errorHandler(exp.id, "Unknown identifier " + name);
  }

  auto var = boost::get<VariableType>(&*val);
  if (!var) {
    return m_errorHandler(exp.id, name + " is not a variable name");
  }

  auto namedType = var->m_type;

  for (const auto& v : exp.rest) {
    if (!helpers::match(v)(
      [&](const ast::VarField& vf)->bool {
      auto record = boost::get<RecordType>(&namedType.m_type);
      if (!record) {
        return m_errorHandler(vf.name.id, "Expression must be of record type");
      }
      auto it = std::find_if(record->m_fields.begin(), record->m_fields.end(), [&](const auto& field) { return field.m_name == vf.name.name; });
      if (it == record->m_fields.end()) {
        return m_errorHandler(vf.name.id, "Unknown record member " + vf.name.name);
      }
      auto tmp = std::move(it->m_type);
      namedType = tmp;
      return true;
    },
      [&](const ast::Subscript& s)->bool {
      auto compiledExp = compile(s.exp);
      if (!compiledExp) {
        return false;
      }
      if (!hasType<IntType>(*compiledExp)) {
        return m_errorHandler(id(s.exp), "Subscript expression must be of integer type");
      }
      auto array = boost::get<ArrayType>(&namedType.m_type);
      if (!array) {
        return m_errorHandler(id(s.exp), "Expression must be of array type");
      }
      auto tmp = array->m_elementType;
      namedType = tmp;
      return true;
    }
      )) {
      return {};
    }
  }

  return CompiledExpression{ namedType };
}

Compiler::result_type Compiler::operator()(const ast::IntExpression & exp)
{
  return CompiledExpression{ s_intType };
}

Compiler::result_type Compiler::operator()(const ast::StringExpression & exp)
{
  return CompiledExpression{ s_stringType };
}

Compiler::result_type Compiler::operator()(const ast::CallExpression & exp)
{
  std::string name = exp.func;
  auto val = findValue(name);
  if (!val) {
    return m_errorHandler(exp.func.id, "Unknown function " + name);
  }
  auto func = boost::get<FunctionType>(&*val);
  if (!func) {
    return m_errorHandler(exp.func.id, name + " is not a function name");
  }

  const auto& paramTypes = func->m_parameterTypes;
  if (exp.args.size() != paramTypes.size()) {
    return m_errorHandler(exp.func.id,
      "Expecting " + std::to_string(paramTypes.size()) + " parameters, " + std::to_string(exp.args.size()) + " given");
  }

  std::vector<NamedType> argTypes;
  for (const auto& arg : exp.args)
  {
    auto argType = compile(arg);
    if (!argType) {
      return {};
    }
    argTypes.push_back(argType->m_type);
  }

  for (size_t i = 0; i < argTypes.size(); ++i) {
    if (!equalTypes(argTypes[i], paramTypes[i])) {
      return m_errorHandler(id(exp.args[i]), "Wrong type of parameter " + std::to_string(i) + ", expecting " +
        paramTypes[i].m_name + ", got " + argTypes[i].m_name);
    }
  }

  return CompiledExpression{ func->m_resultType };
}

Compiler::result_type Compiler::operator()(const ast::ArithmeticExpression & exp)
{
  auto lhs = compile(exp.first);
  if (!lhs) {
    return {};
  }

  auto checkType = [this](ast::Operation operation, 
                          const CompiledExpression& exp,
                          size_t id)->bool
  {
    switch (operation)
    {
      case ast::Operation::EQUAL:
      case ast::Operation::NOT_EQUAL:
        // any type is OK
        break;

      case ast::Operation::LESS_THEN:
      case ast::Operation::LESS_EQUAL:
      case ast::Operation::GREATER_THEN:
      case ast::Operation::GREATER_EQUAL:
        // int or string
        if (!hasType<IntType>(exp) && !hasType<StringType>(exp))
        {
          return m_errorHandler(id, "Expression must be of type int or string");
        }
        break;

      default:
        // other operations must have int type
        if (!hasType<IntType>(exp))
        {
          return m_errorHandler(id, "Expression must be of type int");
        }
    }

    return true;
  };

  if (!exp.rest.empty()
      && !checkType(exp.rest[0].op, *lhs, id(exp.first)))
  {
    return {};
  }

  for (const auto& opExp : exp.rest)
  {
    auto rhs = compile(opExp.exp);
    if (!rhs) {
      return rhs;
    }

    if (!checkType(opExp.op, *rhs, id(opExp.exp))) {
      return {};
    }

    if (!equalTypes(*rhs, *lhs)) {
      return m_errorHandler(id(opExp.exp), "Both operands must have the same type");
    }

    return CompiledExpression{ s_intType };
  }

  return {};
}

Compiler::result_type Compiler::operator()(const ast::RecordExpression & exp)
{
  auto type = findType(exp.type);
  if (!type) {
    return m_errorHandler(exp.type.id, "Undeclared type " + exp.type.name);
  }

  auto recordType = boost::get<RecordType>(&type->m_type);
  if (!recordType) {
    return m_errorHandler(exp.type.id, type->m_name + " must be a record type");
  }

  const auto& recordFields = recordType->m_fields;
  if (exp.fields.size() != recordFields.size()) {
    return m_errorHandler(exp.type.id,
      "Expecting " + std::to_string(recordFields.size()) + " fields, " + std::to_string(exp.fields.size()) + " given");
  }

  std::vector<NamedType> fieldTypes;
  for (const auto& field : exp.fields)
  {
    auto fieldType = compile(field.exp);
    if (!fieldType) {
      return {};
    }
    fieldTypes.push_back(fieldType->m_type);
  }

  for (size_t i = 0; i < fieldTypes.size(); ++i) {
    if (!equalTypes(fieldTypes[i], recordFields[i].m_type)) {
      return m_errorHandler(exp.fields[i].name.id, "Wrong type of field " + std::to_string(i) + " expecting "
        + recordFields[i].m_type.m_name + ", got " + fieldTypes[i].m_name);
    }
  }

  return CompiledExpression{ *type };
}

Compiler::result_type Compiler::operator()(const ast::AssignExpression & exp)
{
  auto varType = compile(exp.var);
  if (!varType) {
    return {};
  }

  auto expType = compile(exp.exp);
  if (!expType) {
    return {};
  }

  if (!equalTypes(*varType, *expType)) {
    return m_errorHandler(id(exp.var), "Type " + expType->m_type.m_name + " cannot be assigned to type " + varType->m_type.m_name);
  }

  return CompiledExpression{ s_voidType };
}

Compiler::result_type Compiler::operator()(const ast::IfExpression & exp)
{
  auto test = compile(exp.test);
  if (!test) {
    return {};
  }

  if (!hasType<IntType>(*test)) {
    return m_errorHandler(id(exp.test), "Expression must have int type");
  }

  auto thenExp = compile(exp.thenExp);
  if (!thenExp) {
    return {};
  }

  if (exp.elseExp)
  {
    auto elseExp = compile(*exp.elseExp);
    if (!elseExp) {
      return {};
    }

    if (!equalTypes(*thenExp, *elseExp)) {
      // handle int type specially so to not confuse the user when a boolean expression is converted to an if
      // and the second expression is generated by the parser
      if (hasType<IntType>(*thenExp)) {
        return m_errorHandler(id(*exp.elseExp), "Expression must have int type");
      }
      if (hasType<IntType>(*elseExp)) {
        return m_errorHandler(id(exp.thenExp), "Expression must have int type");
      }
      
      return m_errorHandler(id(exp), "Both expressions must have the same type");
    }
  }
  else
  {
    if (!hasType<VoidType>(*thenExp)) {
      return m_errorHandler(id(exp.thenExp), "Expression must produce no value");
    }
  }

  return CompiledExpression{ thenExp->m_type };
}

Compiler::result_type Compiler::operator()(const ast::WhileExpression & exp)
{
  auto test = compile(exp.test);
  if (!test) {
    return {};
  }
  if (!hasType<IntType>(*test)) {
    return m_errorHandler(id(exp.test), "Expression must be of type int");
  }

  m_withinLoop = true;

  auto body = compile(exp.body);
  if (!body) {
    return {};
  }

  if (!hasType<VoidType>(*body)) {
    return m_errorHandler(id(exp.body), "Expression must produce no value");
  }

  m_withinLoop = false;

  return CompiledExpression{ s_voidType };
}

Compiler::result_type Compiler::operator()(const ast::BreakExpression & exp)
{
  if (!m_withinLoop) {
    return m_errorHandler(id(exp), "break is only legal within a while or a for loop");
  };

  return CompiledExpression{ s_voidType };
}

Compiler::result_type Compiler::operator()(const ast::ForExpression & exp)
{
  auto fromExp = compile(exp.lo);
  if (!fromExp) {
    return {};
  }

  if (!hasType<IntType>(*fromExp)) {
    return m_errorHandler(id(exp.lo), "Expression must be of type int");
  }

  auto toExp = compile(exp.hi);
  if (!toExp) {
    return {};
  }

  if (!hasType<IntType>(*toExp)) {
    return m_errorHandler(id(exp.hi), "Expression must be of type int");
  }

  Environment newEnv;
  newEnv.m_valueMap[exp.var] = VariableType{ s_intType };
  m_environments.push_back(newEnv);

  m_withinLoop = true;

  auto bodyExp = compile(exp.body);

  m_withinLoop = false;

  m_environments.pop_back();

  if (!bodyExp) {
    return {};
  }

  if (!hasType<VoidType>(*bodyExp)) {
    return m_errorHandler(id(exp.body), "Expression must produce no value");
  }

  return CompiledExpression{ s_voidType };
}

Compiler::result_type Compiler::operator()(const ast::LetExpression & exp)
{
  m_environments.emplace_back();

  for (const auto& dec : exp.decs)
  {
    if (!addToEnv(dec)) {
      return {};
    }
  }

  result_type res = CompiledExpression{ s_voidType };
  for (const auto& e : exp.body) {
    res = compile(e);
    if (!res) {
      return {};
    }
  }

  m_environments.pop_back();

  return res;
}

Compiler::result_type Compiler::operator()(const ast::ArrayExpression & exp)
{
  auto type = findType(exp.type);
  if (!type) {
    return m_errorHandler(exp.type.id, "Undeclared type " + exp.type.name);
  }

  auto arrayType = boost::get<ArrayType>(&type->m_type);
  if (!arrayType) {
    return m_errorHandler(exp.type.id, type->m_name + " must be an array type");
  }

  auto sizeExp = compile(exp.size);
  if (!sizeExp) {
    return {};
  }

  if (!hasType<IntType>(*sizeExp)) {
    return m_errorHandler(id(exp.size), "Array size must be of type int");
  }

  auto initExp = compile(exp.init);
  if (!initExp) {
    return {};
  }

  if (!equalTypes(arrayType->m_elementType, initExp->m_type)) {
    return m_errorHandler(id(exp.init), "Cannot initialize an array of " + arrayType->m_elementType.m_name
      + " from type " + initExp->m_type.m_name);
  }

  return CompiledExpression{ *type };
}

Compiler::result_type Compiler::operator()(const ast::ExpressionSequence & exp)
{
  CompiledExpression res{ s_voidType };

  for (const auto& e : exp.exps)
  {
    auto compiled = compile(e);
    if (!compiled) {
      return {};
    }

    res.m_type = compiled->m_type;
  }

  return res;
}

bool Compiler::addToEnv(const ast::FunctionDeclarations & decs)
{
  // first pass, compile function signatures
  // to support recursive functions
  for (const auto& dec : decs)
  {
    FunctionType function;
    if (dec.result) {
      auto type = findType(*dec.result);
      if (!type) {
        return m_errorHandler(dec.name.id, "Undeclared type " + dec.result->name);
      }
      function.m_resultType = *type;
    }
    else {
      function.m_resultType = s_voidType;
    }

    for (auto param : dec.params)
    {
      auto type = findType(param.type);
      if (!type) {
        return m_errorHandler(dec.name.id, "Undeclared type " + param.type.name);
      }

      function.m_parameterTypes.push_back(*type);
    }

    addToEnv(dec.name, function);
  }

  // second pass, compile function bodies
  for (const auto& dec : decs) 
  {
    auto function = findValue(dec.name);
    assert(function);

    auto funcType = boost::get<FunctionType>(&*function);
    assert(funcType);

    m_environments.emplace_back();

    for (size_t i = 0; i < dec.params.size();++i)
    {
      addToEnv(dec.params[i].name, VariableType{ funcType->m_parameterTypes[i] });
    }

    auto compiled = compile(dec.body);
    if (!compiled) {
      return false;
    }

    m_environments.pop_back();

    if (!equalTypes(compiled->m_type, funcType->m_resultType)) {
      return m_errorHandler(id(dec.body), "Function body type must be " + funcType->m_resultType.m_name);
    }
  }

  return true;
}

bool Compiler::addToEnv(const ast::VarDeclaration & dec)
{
  auto compiled = compile(dec.init);
  if (!compiled) {
    return false;
  }

  VariableType var;

  if (dec.type) {
    auto type = findType(dec.type->name);
    if (!type) {
      return m_errorHandler(dec.type->id, "Undeclared type " + dec.type->name);
    }

    if (!equalTypes(*type, compiled->m_type)) {
      return m_errorHandler(id(dec.init), "Type of initializing expression must be " + type->m_name);
    }

    var.m_type = *type;
  }
  else if (hasType<NilType>(*compiled)) {
    return m_errorHandler(id(dec.init), "nil can only be used in a context where its type can be determined");
  }
  else {
    var.m_type = compiled->m_type;
  }

  addToEnv(dec.name, var);

  return true;
}

bool Compiler::addToEnv(const ast::TypeDeclarations & decs)
{
  // first pass, add type names to environment
  // to support recursive types
  for (const auto& dec : decs)
  {
    addToEnv(dec.name, NamedType{ dec.name, VoidType{} });
  }

  // second pass, compile declarations
  for (const auto& dec : decs)
  {
    using MatchRes = boost::optional<Type>;
    auto type = helpers::match(dec.type)(
      [&](const ast::NameType& nameType)->MatchRes {
      auto type = findType(nameType.type);
      if (!type) {
        return m_errorHandler(nameType.type.id, "Undeclared type " + nameType.type.name);
      }

      if (type->m_name == dec.name.name) {
        return m_errorHandler(dec.name.id, "Cyclic recursive deceleration");
      }

      return NameType{ type->m_name };
    },
      [&](const ast::ArrayType& arrayType)->MatchRes {
      auto type = findType(arrayType.type);
      if (!type) {
        return m_errorHandler(arrayType.type.id, "Undeclared type " + arrayType.type.name);
      }

      return ArrayType{ *type };
    },
      [&](const ast::RecordType& recordType)->MatchRes {
      RecordType res;
      for (const auto& field : recordType.fields)
      {
        auto type = findType(field.type);
        if (!type) {
          return m_errorHandler(field.type.id, "Undeclared type " + field.type.name);
        }
        res.m_fields.push_back({ field.name, *type });
      }

      return res;
    }
    );

    if (!type) {
      return false;
    }

    addToEnv(dec.name, NamedType{ dec.name, *type });
  }

  // third pass, replace all recursive types with actual types
  for (auto& it : m_environments.back().m_typeMap) {
    if (auto recordType = boost::get<RecordType>(&it.second.m_type)) {
      for (auto& field : recordType->m_fields) {
        if (hasType<VoidType>(field.m_type)) {
          field.m_type = *findType(field.m_type.m_name);
        }
      }
    }
  }

  return true;
}

bool Compiler::addToEnv(const ast::Declaration & dec)
{
  return helpers::match(dec)(
    [&](const ast::FunctionDeclarations& decs) {
    return addToEnv(decs);
  },
    [&](const ast::VarDeclaration& dec) {
    return addToEnv(dec);
  },
    [&](const ast::TypeDeclarations& decs) {
    return addToEnv(decs);
  }
  );
}

void Compiler::addToEnv(const TypeMap::key_type& name, const TypeMap::mapped_type& type)
{
  m_environments.back().m_typeMap[name] = type;
}

void Compiler::addToEnv(const ValueMap::key_type& name, const ValueMap::mapped_type& value)
{
  m_environments.back().m_valueMap[name] = value;
}

size_t Compiler::id(const ast::Expression & expression) const
{
  return helpers::match(expression)(
    [&](const ast::Tagged& e) { return e.id; }
  );
}

Compiler::Environment Compiler::defaultEnvironment()
{
  Environment defaultEnv;

  // primitive types
  auto& typeMap = defaultEnv.m_typeMap;

  typeMap["int"] = s_intType;
  typeMap["string"] = s_stringType;

  // standard library
  auto& valueMap = defaultEnv.m_valueMap;

  valueMap["print"] = FunctionType{ s_voidType, {s_stringType} };
  valueMap["flush"] = FunctionType{ s_voidType };
  valueMap["getchar"] = FunctionType{ s_stringType };
  valueMap["ord"] = FunctionType{ s_intType, {s_stringType} };
  valueMap["chr"] = FunctionType{ s_stringType, {s_intType} };
  valueMap["size"] = FunctionType{ s_intType, {s_stringType} };
  valueMap["substring"] = FunctionType{ s_stringType, {s_stringType, s_intType, s_intType} };
  valueMap["concat"] = FunctionType{ s_stringType, {s_stringType, s_stringType} };
  valueMap["not"] = FunctionType{ s_intType, {s_intType} };
  valueMap["exit"] = FunctionType{ s_voidType, {s_intType} };

  return defaultEnv;
}

Compiler::OptionalValue Compiler::findValue(const std::string & name) const
{
  for (auto it = m_environments.rbegin(); it != m_environments.rend(); ++it) {
    auto itt = it->m_valueMap.find(name);
    if (itt != it->m_valueMap.end()) {
      return itt->second;
    }
  }

  return {};
}

tiger::Compiler::OptionalType Compiler::findType(std::string name) const
{
  for (auto it = m_environments.rbegin(); it != m_environments.rend(); ++it) {
    for(auto itt = it->m_typeMap.find(name); itt != it->m_typeMap.end(); itt = it->m_typeMap.find(name)) {
      auto res = &itt->second;
      // go through type names
      if (auto typeName = boost::get<NameType>(&res->m_type)) {
        name = typeName->m_name;
        continue;
      }

      return *res;
    }
  }

  return {};
}

} // namespace tiger