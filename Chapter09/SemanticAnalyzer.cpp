#include "SemanticAnalyzer.h"
#include "CallingConvention.h"
#include "variantMatch.h"
#include <boost/dynamic_bitset.hpp>

namespace tiger {
const NamedType SemanticAnalyzer::s_intType{"int", IntType{}};
const NamedType SemanticAnalyzer::s_stringType{"string", StringType{}};
const NamedType SemanticAnalyzer::s_voidType{"void", VoidType{}};
const NamedType SemanticAnalyzer::s_nilType{"nil", NilType{}};

FragmentList SemanticAnalyzer::compile(const ast::Expression &ast) {
  m_functionLevels.push_back(m_translator.outermost());
  auto compiled = compileExpression(ast);
  m_translator.translateFunction(m_translator.topLevel(), m_tempMap.namedLabel("main"), 
     m_translator.toExpression(compiled.m_translated));
  return m_translator.result();
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::Expression &ast) {
  return helpers::match(ast)(
      [=](auto &exp) { return this->compileExpression(exp); });
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::NilExpression & /*exp*/) {
  return CompiledExpression{s_nilType};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::VarExpression &exp) {
  std::string name = exp.first;
  auto val = findValue(name);
  if (!val) {
    m_errorHandler(exp.id, "Unknown identifier " + name);
  }

  auto var = boost::get<VariableType>(&*val);
  if (!var) {
    m_errorHandler(exp.id, name + " is not a variable name");
  }

  auto namedType = var->m_type;

  auto translatedExp =
      m_translator.translateVar(m_functionLevels, var->m_access);

  for (const auto &v : exp.rest) {
    if (!helpers::match(v)(
            [&](const ast::VarField &vf) -> bool {
              auto record = boost::get<RecordType>(&namedType.m_type);
              if (!record) {
                m_errorHandler(vf.name.id, "Expression must be of record type");
              }
              auto it =
                  std::find_if(record->m_fields.begin(), record->m_fields.end(),
                               [&](const auto &field) {
                                 return field.m_name == vf.name.name;
                               });
              if (it == record->m_fields.end()) {
                m_errorHandler(vf.name.id,
                               "Unknown record member " + vf.name.name);
              }

              // since each member is a scalar this is the same as array access
              translatedExp = m_translator.translateArrayAccess(
                  translatedExp, static_cast<int>(std::distance(
                                     record->m_fields.begin(), it)));

              auto tmp = std::move(it->m_type);
              namedType = tmp;

              return true;
            },
            [&](const ast::Subscript &s) -> bool {
              auto compiledExp = compileExpression(s.exp);
              if (!hasType<IntType>(compiledExp)) {
                m_errorHandler(id(s.exp),
                               "Subscript expression must be of integer type");
              }
              auto array = boost::get<ArrayType>(&namedType.m_type);
              if (!array) {
                m_errorHandler(id(s.exp), "Expression must be of array type");
              }

              translatedExp = m_translator.translateArrayAccess(
                  translatedExp,
                  boost::get<ir::Expression>(compiledExp.m_translated));

              auto tmp = array->m_elementType;
              namedType = tmp;

              return true;
            })) {
      return {};
    }
  }

  return CompiledExpression{namedType, translatedExp};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::IntExpression &exp) {
  return CompiledExpression{s_intType, m_translator.translateConstant(exp.i)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::StringExpression &exp) {
  return CompiledExpression{s_stringType, m_translator.translateString(exp.s)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::CallExpression &exp) {
  std::string name = exp.func;
  auto val = findValue(name);
  if (!val) {
    m_errorHandler(exp.func.id, "Unknown function " + name);
  }
  auto func = boost::get<FunctionType>(&*val);
  if (!func) {
    m_errorHandler(exp.func.id, name + " is not a function name");
  }

  const auto &paramTypes = func->m_parameterTypes;
  if (exp.args.size() != paramTypes.size()) {
    m_errorHandler(exp.func.id, "Expecting " +
                                    std::to_string(paramTypes.size()) +
                                    " parameters, " +
                                    std::to_string(exp.args.size()) + " given");
  }

  std::vector<NamedType> argTypes;
  std::vector<translator::Expression> translatedArgs;
  for (const auto &arg : exp.args) {
    auto argType = compileExpression(arg);
    argTypes.push_back(argType.m_type);
    translatedArgs.push_back(argType.m_translated);
  }

  for (size_t i = 0; i < argTypes.size(); ++i) {
    if (!equalTypes(argTypes[i], paramTypes[i])) {
      m_errorHandler(id(exp.args[i]), "Wrong type of parameter " +
                                          std::to_string(i) + ", expecting " +
                                          paramTypes[i].m_name + ", got " +
                                          argTypes[i].m_name);
    }
  }

  return CompiledExpression{
      func->m_resultType,
      m_translator.translateCall(m_functionLevels, func->m_label, func->m_declerationLevel,
                                 translatedArgs)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::ArithmeticExpression &exp) {
  auto lhs = compileExpression(exp.first);

  auto checkType = [this](ast::Operation operation,
                          const CompiledExpression &exp, size_t id) {
    switch (operation) {
    case ast::Operation::EQUAL:
    case ast::Operation::NOT_EQUAL:
      // any type is OK
      break;

    case ast::Operation::LESS_THEN:
    case ast::Operation::LESS_EQUAL:
    case ast::Operation::GREATER_THEN:
    case ast::Operation::GREATER_EQUAL:
      // int or string
      if (!hasType<IntType>(exp) && !hasType<StringType>(exp)) {
        m_errorHandler(id, "Expression must be of type int or string");
      }
      break;

    default:
      // other operations must have int type
      if (!hasType<IntType>(exp)) {
        m_errorHandler(id, "Expression must be of type int");
      }
    }
  };

  if (!exp.rest.empty()) {
    checkType(exp.rest[0].op, lhs, id(exp.first));
  }

  auto translated = lhs.m_translated;
  auto isString = hasType<StringType>(lhs);

  for (const auto &opExp : exp.rest) {
    auto rhs = compileExpression(opExp.exp);
    checkType(opExp.op, rhs, id(opExp.exp));

    if (!equalTypes(rhs, lhs)) {
      m_errorHandler(id(opExp.exp), "Both operands must have the same type");
    }

    switch (opExp.op) {
    case ast::Operation::PLUS:
    case ast::Operation::MINUS:
    case ast::Operation::TIMES:
    case ast::Operation::DIVIDE:
    case ast::Operation::AND:
    case ast::Operation::OR:
      translated = m_translator.translateArithmetic(
          toBinOp(opExp.op), translated, rhs.m_translated);
      break;
    case ast::Operation::EQUAL:
    case ast::Operation::NOT_EQUAL:
    case ast::Operation::LESS_THEN:
    case ast::Operation::GREATER_THEN:
    case ast::Operation::LESS_EQUAL:
    case ast::Operation::GREATER_EQUAL:
      if (isString) {
        translated = m_translator.translateStringCompare(
            toRelOp(opExp.op), translated, rhs.m_translated);
      } else {
        translated = m_translator.translateRelation(
            toRelOp(opExp.op), translated, rhs.m_translated);
      }
      break;
    default:
      assert(false && "Unexpected operation");
    }
  }

  return CompiledExpression{s_intType, translated};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::RecordExpression &exp) {
  auto type = findType(exp.type);
  if (!type) {
    m_errorHandler(exp.type.id, "Undeclared type " + exp.type.name);
  }

  auto recordType = boost::get<RecordType>(&type->m_type);
  if (!recordType) {
    m_errorHandler(exp.type.id, type->m_name + " must be a record type");
  }

  const auto &recordFields = recordType->m_fields;
  if (exp.fields.size() != recordFields.size()) {
    m_errorHandler(exp.type.id,
                   "Expecting " + std::to_string(recordFields.size()) +
                       " fields, " + std::to_string(exp.fields.size()) +
                       " given");
  }

  std::vector<NamedType> fieldTypes;
  std::vector<translator::Expression> translatedFields;
  for (const auto &field : exp.fields) {
    auto fieldType = compileExpression(field.exp);
    fieldTypes.push_back(fieldType.m_type);
    translatedFields.push_back(fieldType.m_translated);
  }

  for (size_t i = 0; i < fieldTypes.size(); ++i) {
    if (!equalTypes(fieldTypes[i], recordFields[i].m_type)) {
      m_errorHandler(exp.fields[i].name.id,
                     "Wrong type of field " + std::to_string(i) +
                         " expecting " + recordFields[i].m_type.m_name +
                         ", got " + fieldTypes[i].m_name);
    }
  }

  return CompiledExpression{*type,
                            m_translator.translateRecord(translatedFields)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::AssignExpression &exp) {
  auto varType = compileExpression(exp.var);
  auto expType = compileExpression(exp.exp);

  if (!equalTypes(varType, expType)) {
    m_errorHandler(id(exp.var), "Type " + expType.m_type.m_name +
                                    " cannot be assigned to type " +
                                    varType.m_type.m_name);
  }

  return CompiledExpression{
      s_voidType, m_translator.translateAssignment(varType.m_translated,
                                                   expType.m_translated)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::IfExpression &exp) {
  auto test = compileExpression(exp.test);

  if (!hasType<IntType>(test)) {
    m_errorHandler(id(exp.test), "Expression must have int type");
  }

  auto thenExp = compileExpression(exp.thenExp);

  boost::optional<translator::Expression> translatedElse{};
  if (exp.elseExp) {
    auto elseExp = compileExpression(*exp.elseExp);

    if (!equalTypes(thenExp, elseExp)) {
      // handle int type specially so to not confuse the user when a boolean
      // expression is converted to an if and the second expression is
      // generated by the parser
      if (hasType<IntType>(thenExp)) {
        m_errorHandler(id(*exp.elseExp), "Expression must have int type");
      }
      if (hasType<IntType>(elseExp)) {
        m_errorHandler(id(exp.thenExp), "Expression must have int type");
      }

      m_errorHandler(id(exp), "Both expressions must have the same type");
    }

    translatedElse = elseExp.m_translated;
  } else if (!hasType<VoidType>(thenExp)) {
    m_errorHandler(id(exp.thenExp), "Expression must produce no value");
  }

  auto translated = m_translator.translateConditional(
      test.m_translated, thenExp.m_translated, translatedElse);
  return CompiledExpression{thenExp.m_type, translated};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::WhileExpression &exp) {
  auto test = compileExpression(exp.test);
  if (!hasType<IntType>(test)) {
    m_errorHandler(id(exp.test), "Expression must be of type int");
  }

  m_breakTargets.push_back(m_translator.loopDone());

  auto body = compileExpression(exp.body);

  if (!hasType<VoidType>(body)) {
    m_errorHandler(id(exp.body), "Expression must produce no value");
  }

  CompiledExpression res{s_voidType, m_translator.translateWhileLoop(
                                         test.m_translated, body.m_translated,
                                         m_breakTargets.back())};

  m_breakTargets.pop_back();

  return res;
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::BreakExpression &exp) {
  if (m_breakTargets.empty()) {
    m_errorHandler(id(exp), "break is only legal within a while or a for loop");
  };

  return CompiledExpression{s_voidType,
                            m_translator.translateBreak(m_breakTargets.back())};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::ForExpression &exp) {
  auto fromExp = compileExpression(exp.lo);
  if (!hasType<IntType>(fromExp)) {
    m_errorHandler(id(exp.lo), "Expression must be of type int");
  }

  auto toExp = compileExpression(exp.hi);
  if (!hasType<IntType>(toExp)) {
    m_errorHandler(id(exp.hi), "Expression must be of type int");
  }

  VariableType forVar{s_intType, m_translator.allocateLocal(
                                     m_functionLevels.back(), exp.escapes)};

  m_environments.emplace_back();
  addToEnv(exp.var, forVar);

  m_breakTargets.push_back(m_translator.loopDone());

  auto bodyExp = compileExpression(exp.body);

  m_environments.pop_back();

  if (!hasType<VoidType>(bodyExp)) {
    m_errorHandler(id(exp.body), "Expression must produce no value");
  }

  CompiledExpression res{
      s_voidType,
      m_translator.translateForLoop(
          m_translator.translateVar(m_functionLevels, forVar.m_access),
          fromExp.m_translated, toExp.m_translated, bodyExp.m_translated,
          m_breakTargets.back())};

  m_breakTargets.pop_back();

  return res;
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::LetExpression &exp) {
  m_environments.emplace_back();

  std::vector<translator::Expression> decs, exps;
  NamedType resType;
  decs.reserve(exp.decs.size());
  std::transform(exp.decs.begin(), exp.decs.end(), std::back_inserter(decs),
                 [this](const ast::Declaration &dec) { return addToEnv(dec); });

  exps.reserve(exp.body.size());
  std::transform(exp.body.begin(), exp.body.end(), std::back_inserter(exps),
                 [this, &resType](const ast::Expression &exp) {
                   auto compiled = compileExpression(exp);
                   resType = compiled.m_type;
                   return compiled.m_translated;
                 });

  m_environments.pop_back();

  return CompiledExpression{resType, m_translator.translateLet(decs, exps)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::ArrayExpression &exp) {
  auto type = findType(exp.type);
  if (!type) {
    m_errorHandler(exp.type.id, "Undeclared type " + exp.type.name);
  }

  auto arrayType = boost::get<ArrayType>(&type->m_type);
  if (!arrayType) {
    m_errorHandler(exp.type.id, type->m_name + " must be an array type");
  }

  auto sizeExp = compileExpression(exp.size);
  if (!hasType<IntType>(sizeExp)) {
    m_errorHandler(id(exp.size), "Array size must be of type int");
  }

  auto initExp = compileExpression(exp.init);
  if (!equalTypes(arrayType->m_elementType, initExp.m_type)) {
    m_errorHandler(id(exp.init), "Cannot initialize an array of " +
                                     arrayType->m_elementType.m_name +
                                     " from type " + initExp.m_type.m_name);
  }

  return CompiledExpression{
      *type,
      m_translator.translateArray(sizeExp.m_translated, initExp.m_translated)};
}

SemanticAnalyzer::result_type
SemanticAnalyzer::compileExpression(const ast::ExpressionSequence &exp) {
  CompiledExpression res{s_voidType};

  std::vector<translator::Expression> translated;
  translated.reserve(exp.exps.size());
  std::transform(exp.exps.begin(), exp.exps.end(),
                 std::back_inserter(translated),
                 [this, &res](const ast::Expression &exp) {
                   auto compiled = compileExpression(exp);
                   res.m_type = compiled.m_type;
                   return compiled.m_translated;
                 });

  switch (translated.size()) {
  case 0:
    break;
  case 1:
    res.m_translated = translated.front();
    break;
  default:
    // translated same as let with no declarations
    res.m_translated = m_translator.translateLet({}, translated);
    break;
  }

  return res;
}

SemanticAnalyzer::CompiledDeclaration
SemanticAnalyzer::addToEnv(const ast::FunctionDeclarations &decs) {
  // first pass, compile function signatures
  // to support recursive functions
  for (const auto &dec : decs) {
    FunctionType function;
    if (dec.result) {
      auto type = findType(*dec.result);
      if (!type) {
        m_errorHandler(dec.name.id, "Undeclared type " + dec.result->name);
      }
      function.m_resultType = *type;
    } else {
      function.m_resultType = s_voidType;
    }

    frame::BoolList formals;

    for (const auto &param : dec.params) {
      auto type = findType(param.type);
      if (!type) {
        m_errorHandler(dec.name.id, "Undeclared type " + param.type.name);
      }

      formals.push_back(param.escapes);
      function.m_parameterTypes.push_back(*type);
    }

    function.m_label = m_tempMap.newLabel();
    function.m_declerationLevel = m_functionLevels.back();
    function.m_bodyLevel = m_translator.newLevel(function.m_label, formals);
    addToEnv(dec.name, function);
  }

  // second pass, compile function bodies
  for (const auto &dec : decs) {
    auto function = findValue(dec.name);
    assert(function);

    auto funcType = boost::get<FunctionType>(&*function);
    assert(funcType);

    m_environments.emplace_back();
    m_functionLevels.push_back(funcType->m_bodyLevel);

    auto formals = m_translator.formals(funcType->m_bodyLevel);
    for (size_t i = 0; i < dec.params.size(); ++i) {
      addToEnv(dec.params[i].name,
               VariableType{funcType->m_parameterTypes[i], formals[i + 1]});
    }

    auto compiled = compileExpression(dec.body);

    m_functionLevels.pop_back();
    m_environments.pop_back();

    if (!equalTypes(compiled.m_type, funcType->m_resultType)) {
      m_errorHandler(id(dec.body), "Function body type must be " +
                                       funcType->m_resultType.m_name);
    }

    m_translator.translateFunction(funcType->m_bodyLevel, funcType->m_label, compiled.m_translated);
  }

  return {};
}

SemanticAnalyzer::CompiledDeclaration
SemanticAnalyzer::addToEnv(const ast::VarDeclaration &dec) {
  auto compiled = compileExpression(dec.init);

  VariableType var;

  if (dec.type) {
    auto type = findType(dec.type->name);
    if (!type) {
      m_errorHandler(dec.type->id, "Undeclared type " + dec.type->name);
    }

    if (!equalTypes(*type, compiled.m_type)) {
      m_errorHandler(id(dec.init),
                     "Type of initializing expression must be " + type->m_name);
    }

    var.m_type = *type;
  } else if (hasType<NilType>(compiled)) {
    m_errorHandler(
        id(dec.init),
        "nil can only be used in a context where its type can be determined");
  } else if (hasType<VoidType>(compiled)) {
    m_errorHandler(id(dec.init), "expression has no value");
  } else {
    var.m_type = compiled.m_type;
  }

  var.m_access =
      m_translator.allocateLocal(m_functionLevels.back(), dec.escapes);

  addToEnv(dec.name, var);

  return m_translator.translateVarDecleration(
      m_translator.translateVar(m_functionLevels, var.m_access),
      compiled.m_translated);
}

SemanticAnalyzer::CompiledDeclaration
SemanticAnalyzer::addToEnv(const ast::TypeDeclarations &decs) {
  // first pass, add type names to environment
  // to support recursive types
  for (const auto &dec : decs) {
    addToEnv(dec.name, NamedType{dec.name, VoidType{}});
  }

  // second pass, compile declarations
  for (const auto &dec : decs) {
    using MatchRes = boost::optional<Type>;
    auto type = helpers::match(dec.type)(
        [&](const ast::NameType &nameType) -> MatchRes {
          auto type = findType(nameType.type);
          if (!type) {
            m_errorHandler(nameType.type.id,
                           "Undeclared type " + nameType.type.name);
          }

          if (type->m_name == dec.name.name) {
            m_errorHandler(dec.name.id, "Cyclic recursive deceleration");
          }

          return MatchRes{NameType{type->m_name}};
        },
        [&](const ast::ArrayType &arrayType) -> MatchRes {
          auto type = findType(arrayType.type);
          if (!type) {
            m_errorHandler(arrayType.type.id,
                           "Undeclared type " + arrayType.type.name);
          }

          return MatchRes{ArrayType{*type}};
        },
        [&](const ast::RecordType &recordType) -> MatchRes {
          RecordType res;
          for (const auto &field : recordType.fields) {
            auto type = findType(field.type);
            if (!type) {
              m_errorHandler(field.type.id,
                             "Undeclared type " + field.type.name);
            }
            res.m_fields.push_back({field.name, *type});
          }

          return MatchRes{res};
        });

    if (!type) {
      return {};
    }

    addToEnv(dec.name, NamedType{dec.name, *type});
  }

  // third pass, replace all recursive types with actual types
  for (auto &it : m_environments.back().m_typeMap) {
    if (auto recordType = boost::get<RecordType>(&it.second.m_type)) {
      for (auto &field : recordType->m_fields) {
        if (hasType<VoidType>(field.m_type)) {
          field.m_type = *findType(field.m_type.m_name);
        }
      }
    }
  }

  return translator::Expression{};
}

SemanticAnalyzer::CompiledDeclaration
SemanticAnalyzer::addToEnv(const ast::Declaration &dec) {
  return helpers::match(dec)(
      [&](const auto &decs) { return this->addToEnv(decs); });
}

void SemanticAnalyzer::addToEnv(const TypeMap::key_type &name,
                                const TypeMap::mapped_type &type) {
  m_environments.back().m_typeMap[name] = type;
}

void SemanticAnalyzer::addToEnv(const ValueMap::key_type &name,
                                const ValueMap::mapped_type &value) {
  m_environments.back().m_valueMap[name] = value;
}

ir::BinOp SemanticAnalyzer::toBinOp(ast::Operation op) {
  switch (op) {
  case ast::Operation::PLUS:
    return ir::BinOp::PLUS;
  case ast::Operation::MINUS:
    return ir::BinOp::MINUS;
  case ast::Operation::TIMES:
    return ir::BinOp::MUL;
  case ast::Operation::DIVIDE:
    return ir::BinOp::DIV;
  case ast::Operation::AND:
    return ir::BinOp::AND;
  case ast::Operation::OR:
    return ir::BinOp::OR;
  default:
    assert(false && "Unexpected operation");
    return {};
  }
}

ir::RelOp SemanticAnalyzer::toRelOp(ast::Operation op) {
  switch (op) {
  case ast::Operation::EQUAL:
    return ir::RelOp::EQ;
  case ast::Operation::NOT_EQUAL:
    return ir::RelOp::NE;
  case ast::Operation::LESS_THEN:
    return ir::RelOp::LT;
  case ast::Operation::GREATER_THEN:
    return ir::RelOp::GT;
  case ast::Operation::LESS_EQUAL:
    return ir::RelOp::LE;
  case ast::Operation::GREATER_EQUAL:
    return ir::RelOp::GE;
  default:
    assert(false && "Unexpected operation");
    return {};
  }
}

size_t SemanticAnalyzer::id(const ast::Expression &expression) const {
  return helpers::match(expression)([&](const ast::Tagged &e) { return e.id; });
}

SemanticAnalyzer::Environment SemanticAnalyzer::defaultEnvironment() {
  Environment defaultEnv;

  // primitive types
  auto &typeMap = defaultEnv.m_typeMap;

  typeMap["int"] = s_intType;
  typeMap["string"] = s_stringType;

  // standard library
  auto addStandardFunction =
      [this, &defaultEnv](const std::string &name, const NamedType &resultType,
                          const std::vector<NamedType> &paramTypes = {}) {
        defaultEnv.m_valueMap[name] =
            FunctionType{resultType, paramTypes, m_tempMap.namedLabel(name),
                         m_translator.outermost()};
      };

  addStandardFunction("print", s_voidType, {s_stringType});
  addStandardFunction("flush", s_voidType);
  addStandardFunction("getchar", s_stringType);
  addStandardFunction("ord", s_intType, {s_stringType});
  addStandardFunction("chr", s_stringType, {s_intType});
  addStandardFunction("size", s_intType, {s_stringType});
  addStandardFunction("substring", s_stringType,
                      {s_stringType, s_intType, s_intType});
  addStandardFunction("concat", s_stringType, {s_stringType, s_stringType});
  addStandardFunction("not", s_intType, {s_intType});
  addStandardFunction("exit", s_voidType, {s_intType});

  return defaultEnv;
}

SemanticAnalyzer::OptionalValue
SemanticAnalyzer::findValue(const std::string &name) const {
  for (auto it = m_environments.rbegin(); it != m_environments.rend(); ++it) {
    auto itt = it->m_valueMap.find(name);
    if (itt != it->m_valueMap.end()) {
      return itt->second;
    }
  }

  return {};
}

SemanticAnalyzer::OptionalType
SemanticAnalyzer::findType(std::string name) const {
  for (auto it = m_environments.rbegin(); it != m_environments.rend(); ++it) {
    for (auto itt = it->m_typeMap.find(name); itt != it->m_typeMap.end();
         itt = it->m_typeMap.find(name)) {
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
