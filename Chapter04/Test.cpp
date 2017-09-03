#include "Program.h"
#include "testsHelper.h"
#include "AbstractSyntaxTree.h"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <boost/variant/get.hpp>
#include <boost/format.hpp>

using namespace tiger;
using boost::get;

TEST_CASE("parse test files") {
  namespace fs = boost::filesystem;
  forEachTigerTest([](const fs::path &filepath, bool parseError, bool compilationError) {
    auto filename = filepath.filename();
    CAPTURE(filename);
    ast::Expression ast;
    if (parseError)
    {
      REQUIRE_FALSE(parseFile(filepath.string(), ast));
    }
    else
    {
      REQUIRE(parseFile(filepath.string(), ast));
    }
  });
}

template<typename... Funcs, typename T, std::size_t... I>
void applyFunctionTupleToVector(const std::tuple<Funcs...>& funcs, const std::vector<T>& v, std::index_sequence<I...>) {
  std::initializer_list<int>{(std::get<I>(funcs)(v[I]), 0)...};
}

template<typename... Funcs, typename T>
void applyFunctionTupleToVector(const std::tuple<Funcs...>& funcs, const std::vector<T>& v) {
  REQUIRE(sizeof...(Funcs) == v.size());
  applyFunctionTupleToVector(funcs, v, std::make_index_sequence<sizeof...(Funcs)>());
}

auto checkIdentifier(const std::string& name)
{
  return [name](const ast::Identifier& id)
  {
    REQUIRE(id.name == name);
  };
}

template<typename... CheckRest>
auto checkLValue(const std::string& identifier, CheckRest&&... checkRest)
{
  return [&](ast::Expression ast)
  {
    auto pVar = get<ast::VarExpression>(&ast);
    REQUIRE(pVar);
    checkIdentifier(identifier)(pVar->first);
    applyFunctionTupleToVector(std::forward_as_tuple(checkRest...), pVar->rest);
  };
}

auto checkVarField(const std::string& name)
{
  return [name](const ast::VarRestElement& varRest)
  {
    auto pField = get<ast::VarField>(&varRest);
    REQUIRE(pField);
    checkIdentifier(name)(pField->name);
  };
}

template<typename CheckExp>
auto checkVarSubscript(CheckExp&& checkExp)
{
  return [&](const ast::VarRestElement&varRest)
  {
    auto pSubscript = get<ast::Subscript>(&varRest);
    REQUIRE(pSubscript);
    checkExp(pSubscript->exp);
  };
}

TEST_CASE("lvalue") {
  ast::Expression ast;
  SECTION("identifier") {
    REQUIRE(parse("i", ast));
    checkLValue("i")(ast);
  }
  SECTION("field") {
    REQUIRE(parse("i.m", ast));
    checkLValue("i", checkVarField("m"))(ast);
  }
  SECTION("array element") {
    REQUIRE(parse("i[m]", ast));
    checkLValue("i", checkVarSubscript(checkLValue("m")))(ast);
  }
}

auto checkNil()
{
  return [](const ast::Expression& ast)
  {
    REQUIRE(get<ast::NilExpression>(&ast));
  };
}

TEST_CASE("nil") {
  ast::Expression ast;
  REQUIRE(parse("nil", ast));
  checkNil()(ast);
}

template<typename... CheckExps>
auto checkSequence(CheckExps&&... checkExps)
{
  return [&](const ast::Expression& ast)
  {
    auto pSequence = get<ast::ExpressionSequence>(&ast);
    REQUIRE(pSequence);
    applyFunctionTupleToVector(std::forward_as_tuple(checkExps...), pSequence->exps);
  };
}

TEST_CASE("sequence") {
  ast::Expression ast;
  auto getSequence = [](const ast::Expression& ast) {
  };
  SECTION("empty") {
    REQUIRE(parse("()", ast));
    checkSequence()(ast);
  }
  SECTION("single expression") {
    REQUIRE(parse("(i)", ast));
    checkSequence(checkLValue("i"))(ast);
  }
  SECTION("multiple expressions") {
    REQUIRE(parse("(i;j)", ast));
    checkSequence(
      checkLValue("i"),
      checkLValue("j")
    );
  }
}

auto checkInt(uint32_t value) {
  return [value](const ast::Expression& ast) {
    auto pInt = get<ast::IntExpression>(&ast);
    REQUIRE(pInt);
    REQUIRE(pInt->i == value);
  };
}

template<typename CheckLhs, typename... CheckOps>
auto checkArithmetic(CheckLhs&& checkLhs, CheckOps&&... checkOps)
{
  return[&](const ast::Expression& ast)
  {
    auto pArithmetic = get<ast::ArithmeticExpression>(&ast);
    REQUIRE(pArithmetic);
    checkLhs(pArithmetic->first);
    applyFunctionTupleToVector(std::forward_as_tuple(checkOps...), pArithmetic->rest);
  };
};

template<typename CheckExp>
auto checkOperation(ast::Operation op, CheckExp&& checkExp) {
  return [op, &checkExp](const ast::OperationExpression& opExp) {
    REQUIRE(opExp.op == op);
    checkExp(opExp.exp);
  };
}

TEST_CASE("integer") {
  ast::Expression ast;
  SECTION("positive") {
    REQUIRE(parse("42", ast));
    checkInt(42)(ast);
  }
  SECTION("negative") {
    REQUIRE(parse("-42", ast));
    // -x is parsed as 0-x
    checkArithmetic(checkInt(0), checkOperation(ast::Operation::MINUS, checkInt(42)))(ast);
  }
}

auto checkString(const std::string& s)
{
  return [s](const ast::Expression& ast) {
    auto pString = get<ast::StringExpression>(&ast);
    REQUIRE(pString);
    REQUIRE(pString->s == s);
  };
}

TEST_CASE("string") {
  ast::Expression ast;
  SECTION("empty") {
    REQUIRE(parse(R"("")", ast));
    checkString("")(ast);
  }
  SECTION("escape sequences") {
    // REQUIRE has troubles with some string literals on VS
    parse(R"("\n")", ast);
    checkString("\n")(ast);
    parse(R"("\t")", ast);
    checkString("\t")(ast);
    parse(R"("\^1")", ast);
    checkString("\x1")(ast);
    parse((boost::format(R"("\%|03|")") % +'0').str(), ast);
    checkString("0")(ast);
    parse(R"("\"")", ast);
    checkString("\"")(ast);
    parse(R"("\\")", ast);
    checkString("\\")(ast);
  }
  SECTION("multi line") {
    parse(R"("abc\ 
      \efg")", ast);
    checkString("abcefg")(ast);
  }
  SECTION("combined") {
    auto program = R"("\tHello \"World\"!\n")";
    REQUIRE(parse(program, ast));
    checkString("\tHello \"World\"!\n")(ast);
  }
}

template<typename... CheckArgs>
auto checkFunctionCall(const std::string& func, CheckArgs&&... checkArgs)
{
  return [&, func](const ast::Expression& ast) {
    auto pCall = get<ast::CallExpression>(&ast);
    REQUIRE(pCall);
    checkIdentifier(func)(pCall->func);
    applyFunctionTupleToVector(std::forward_as_tuple(checkArgs...), pCall->args);
  };
}

TEST_CASE("function call") {
  ast::Expression ast;
  SECTION("with no arguments") {
    REQUIRE(parse("f()", ast));
    checkFunctionCall("f")(ast);
  }
  SECTION("with arguments") {
    REQUIRE(parse("f(a)", ast));
    checkFunctionCall("f", checkLValue("a"))(ast);
  }
  SECTION("with subexpressions") {
    REQUIRE(parse(R"(f(42, "hello"))", ast));
    checkFunctionCall("f", checkInt(42), checkString("hello"))(ast);
  }
}

TEST_CASE("arithmetic") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("2+3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::PLUS, checkInt(3)))(ast);
    REQUIRE(parse("2-3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::MINUS, checkInt(3)))(ast);
    REQUIRE(parse("2*3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::TIMES, checkInt(3)))(ast);
    REQUIRE(parse("2/3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::DIVIDE, checkInt(3)))(ast);
  }
  SECTION("precedence") {
    REQUIRE(parse("2*3+5-1/24", ast));
    checkArithmetic(
      checkArithmetic(checkInt(2), checkOperation(ast::Operation::TIMES, checkInt(3))),
      checkOperation(ast::Operation::PLUS, checkInt(5)),
      checkOperation(ast::Operation::MINUS,
        checkArithmetic(checkInt(1), checkOperation(ast::Operation::DIVIDE, checkInt(24)))
      )
    )(ast);
  }
  SECTION("with subexpression") {
    REQUIRE(parse("f(2)+a/(4 + b)", ast));
    checkArithmetic(
      [](const ast::Expression& ast) {
      REQUIRE(get<ast::CallExpression>(&ast));
    },
      checkOperation(ast::Operation::PLUS,
        [&](const ast::Expression& ast) {
      checkArithmetic(
        [](const ast::Expression& ast) {
        REQUIRE(get<ast::VarExpression>(&ast));
      },
        checkOperation(ast::Operation::DIVIDE,
          [&](const ast::Expression& ast) {
        auto pSequence = get<ast::ExpressionSequence>(&ast);
        REQUIRE(pSequence);
        REQUIRE(pSequence->exps.size() == 1);
        checkArithmetic(checkInt(4),
          checkOperation(ast::Operation::PLUS,
            [](const ast::Expression& ast) {
          REQUIRE(get<ast::VarExpression>(&ast));
        })
        )(pSequence->exps[0]);
      })
        );
    })
      )(ast);
  }
}

TEST_CASE("comparison") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("2=3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::EQUAL, checkInt(3)))(ast);
    REQUIRE(parse("2<>3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::NOT_EQUAL, checkInt(3)))(ast);
    REQUIRE(parse("2>3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::GREATER_THEN, checkInt(3)))(ast);
    REQUIRE(parse("2<3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::LESS_THEN, checkInt(3)))(ast);
    REQUIRE(parse("2>=3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::GREATER_EQUAL, checkInt(3)))(ast);
    REQUIRE(parse("2<=3", ast));
    checkArithmetic(checkInt(2), checkOperation(ast::Operation::LESS_EQUAL, checkInt(3)))(ast);
  }
  SECTION("precedence") {
    REQUIRE(parse("2*5<3+4", ast));
    checkArithmetic(
      checkArithmetic(checkInt(2), checkOperation(ast::Operation::TIMES, checkInt(5))),
      checkOperation(ast::Operation::LESS_THEN,
        checkArithmetic(checkInt(3), checkOperation(ast::Operation::PLUS, checkInt(4)))
      )
    )(ast);
    REQUIRE_FALSE(parse("2=3=4", ast));
  }
  SECTION("with subexpression") {
    REQUIRE(parse("(f(2)<>a)=(4>=2)", ast));
    checkArithmetic(
      [](const ast::Expression& ast) {
      auto pSequence = get<ast::ExpressionSequence>(&ast);
      REQUIRE(pSequence);
      REQUIRE(pSequence->exps.size() == 1);
      checkArithmetic(
        [](const ast::Expression& ast) {
        REQUIRE(get<ast::CallExpression>(&ast));
      },
        checkOperation(ast::Operation::NOT_EQUAL,
          [](const ast::Expression& ast) {
        REQUIRE(get<ast::VarExpression>(&ast));
      }
        )
        )(pSequence->exps[0]);
    },
      checkOperation(ast::Operation::EQUAL,
        [](const ast::Expression& ast) {
      auto pSequence = get<ast::ExpressionSequence>(&ast);
      REQUIRE(pSequence);
      REQUIRE(pSequence->exps.size() == 1);
      checkArithmetic(
        checkInt(4),
        checkOperation(ast::Operation::GREATER_EQUAL, checkInt(2))
      )(pSequence->exps[0]);
    }
      )
      )(ast);
  }
}

template<typename CheckTest, typename CheckThen, typename CheckElse = decltype(checkNil())>
auto checkIf(CheckTest&& checkTest, CheckThen&& checkThen, CheckElse&& checkElse = checkNil())
{
  return [=](const ast::Expression& ast)
  {
    auto pIf = get<ast::IfExpression>(&ast);
    REQUIRE(pIf);
    checkTest(pIf->test);
    checkThen(pIf->thenExp);
    checkElse(pIf->elseExp);
  };
}

// a&b is translated to if a then b else 0
template<typename CheckLhs, typename CheckRhs>
auto checkAnd(CheckLhs&& checkLhs, CheckRhs&& checkRhs)
{
  return checkIf(std::forward<decltype(checkLhs)>(checkLhs), std::forward<decltype(checkRhs)>(checkRhs), checkInt(0));
};

// a|b is translated to if a then 1 else b
template<typename CheckLhs, typename CheckRhs>
auto checkOr(CheckLhs&& checkLhs, CheckRhs&& checkRhs)
{
  return checkIf(std::forward<decltype(checkLhs)>(checkLhs), checkInt(1), std::forward<decltype(checkRhs)>(checkRhs));
};

TEST_CASE("boolean") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("2&3", ast));
    checkAnd(checkInt(2), checkInt(3))(ast);
    REQUIRE(parse("2|3", ast));
    checkOr(checkInt(2), checkInt(3))(ast);
  }
  SECTION("compound") {
    REQUIRE(parse("2&(3|(2-4))", ast));
    checkAnd(
      checkInt(2),
      checkSequence(
        checkOr(
          checkInt(3),
          checkSequence(
            checkArithmetic(
              checkInt(2),
              checkOperation(
                ast::Operation::MINUS,
                checkInt(4)
              )
            )
          )
        )
      )
    )(ast);
  }
  SECTION("with subexpression") {
    REQUIRE(parse("f(2)&(3=4|2/b)", ast));
    checkAnd(
      checkFunctionCall(
        "f",
        checkInt(2)
      ),
      checkSequence(
        checkOr(
          checkArithmetic(
            checkInt(3),
            checkOperation(
              ast::Operation::EQUAL,
              checkInt(4)
            )
          ),
          checkArithmetic(
            checkInt(2),
            checkOperation(
              ast::Operation::DIVIDE,
              checkLValue("b")
            )
          )
        )
      )
    )(ast);
  }
}

template<typename CheckExp>
auto checkRecordField(const std::string& name, CheckExp&& checkExp)
{
  return [&, name](const ast::RecordField& field)
  {
    checkIdentifier(name)(field.name);
    checkExp(field.exp);
  };
}

template<typename... CheckFields>
auto checkRecord(const std::string& type, CheckFields&&... checkFields)
{
  return [&, type](const ast::Expression& ast)
  {
    auto pRecord = get<ast::RecordExpression>(&ast);
    REQUIRE(pRecord);
    checkIdentifier(type)(pRecord->type);
    applyFunctionTupleToVector(std::forward_as_tuple(checkFields...), pRecord->fields);
  };
}

TEST_CASE("record") {
  ast::Expression ast;
  SECTION("empty") {
    REQUIRE(parse("t{}", ast));
    checkRecord("t")(ast);
  }
  SECTION("not empty") {
    REQUIRE(parse("t{a=2}", ast));
    checkRecord("t", checkRecordField("a", checkInt(2)))(ast);
    REQUIRE(parse("tt{a=3, b=a}", ast));
    checkRecord(
      "tt",
      checkRecordField("a", checkInt(3)),
      checkRecordField("b", checkLValue("a"))
    )(ast);
  }
  SECTION("with subexpression") {
    REQUIRE(parse("t{a=f(2+3), b=(a&(b/2))}", ast));
    checkRecord(
      "t",
      checkRecordField(
        "a",
        checkFunctionCall(
          "f",
          checkArithmetic(
            checkInt(2),
            checkOperation(ast::Operation::PLUS, checkInt(3))
          )
        )
      ),
      checkRecordField(
        "b",
        checkSequence(
          checkAnd(
            checkLValue("a"),
            checkSequence(
              checkArithmetic(
                checkLValue("b"),
                checkOperation(ast::Operation::DIVIDE, checkInt(2))
              )
            )
          )
        )
      )
    )(ast);
  }
}

template<typename CheckSize, typename CheckInit>
auto checkArray(const std::string& type, CheckSize&& checkSize, CheckInit&& checkInit)
{
  return [=](const ast::Expression& ast)
  {
    auto pArray = get<ast::ArrayExpression>(&ast);
    checkIdentifier(type)(pArray->type);
    checkSize(pArray->size);
    checkInit(pArray->init);
  };
}

TEST_CASE("array") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("a[2] of 3", ast));
    checkArray(
      "a",
      checkInt(2),
      checkInt(3)
    )(ast);
  }
  SECTION("with subexpression") {
    REQUIRE(parse("a[f(2+b)] of \"hello\"", ast));
    checkArray(
      "a",
      checkFunctionCall(
        "f",
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::PLUS, checkLValue("b"))
        )
      ),
      checkString("hello")
    )(ast);
  }
}

template<typename CheckVar, typename CheckExp>
auto checkAssignment(CheckVar&& checkVar, CheckExp&& checkExp)
{
  return [=](const ast::Expression& ast)
  {
    auto pAssignment = get<ast::AssignExpression>(&ast);
    REQUIRE(pAssignment);
    checkVar(pAssignment->var);
    checkExp(pAssignment->exp);
  };
}

TEST_CASE("assignment") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("a := 3", ast));
    checkAssignment(
      checkLValue("a"),
      checkInt(3)
    )(ast);
  }
  SECTION("with subexpression") {
    REQUIRE(parse("a[b.d] := f(5/d)", ast));
    checkAssignment(
      checkLValue(
        "a",
        checkVarSubscript(
          checkLValue(
            "b",
            checkVarField("d")
          )
        )
      ),
      checkFunctionCall(
        "f",
        checkArithmetic(
          checkInt(5),
          checkOperation(ast::Operation::DIVIDE, checkLValue("d"))
        )
      )
    )(ast);
  }
}

TEST_CASE("if then") {
  ast::Expression ast;
  SECTION("no else") {
    REQUIRE(parse("if a then a := 3", ast));
    checkIf(
      checkLValue("a"),
      checkAssignment(
        checkLValue("a"),
        checkInt(3)
      )
    )(ast);
    SECTION("with subexpression") {
      REQUIRE(parse("if f(b)=2/d then a[2] := g(b.d)", ast));
      checkIf(
        checkArithmetic(
          checkFunctionCall(
            "f",
            checkLValue("b")
          ),
          checkOperation(
            ast::Operation::EQUAL,
            checkArithmetic(
              checkInt(2),
              checkOperation(ast::Operation::DIVIDE, checkLValue("d"))
            )
          )
        ),
        checkAssignment(
          checkLValue("a", checkVarSubscript(checkInt(2))),
          checkFunctionCall(
            "g",
            checkLValue("b", checkVarField("d"))
          )
        )
      )(ast);
    }
    SECTION("nested") {
      REQUIRE(parse("if 2>3 then if 2<3 then a := 3", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkIf(
          checkArithmetic(
            checkInt(2),
            checkOperation(ast::Operation::LESS_THEN, checkInt(3))
          ),
          checkAssignment(
            checkLValue("a"),
            checkInt(3)
          )
        )
      )(ast);
      REQUIRE(parse("if 2>3 then (if 2<3 then a := 3)", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkSequence(
          checkIf(
            checkArithmetic(
              checkInt(2),
              checkOperation(ast::Operation::LESS_THEN, checkInt(3))
            ),
            checkAssignment(
              checkLValue("a"),
              checkInt(3)
            )
          )
        )
      )(ast);
    }
  }
  SECTION("with else") {
    REQUIRE(parse("if a then a := 3 else a := 2", ast));
    checkIf(
      checkLValue("a"),
      checkAssignment(
        checkLValue("a"),
        checkInt(3)
      ),
      checkAssignment(
        checkLValue("a"),
        checkInt(2)
      )
    )(ast);
    REQUIRE(parse("if a then 5 else 7", ast));
    checkIf(
      checkLValue("a"),
      checkInt(5),
      checkInt(7)
    )(ast);
    SECTION("with subexpression") {
      REQUIRE(parse("if f(b)=2/d then 2/3 else f(5)-b", ast));
      checkIf(
        checkArithmetic(
          checkFunctionCall(
            "f",
            checkLValue("b")
          ),
          checkOperation(
            ast::Operation::EQUAL,
            checkArithmetic(checkInt(2), checkOperation(ast::Operation::DIVIDE, checkLValue("d")))
          )
        ),
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::DIVIDE, checkInt(3))
        ),
        checkArithmetic(
          checkFunctionCall("f", checkInt(5)),
          checkOperation(ast::Operation::MINUS, checkLValue("b"))
        )
      )(ast);
    }
    SECTION("nested") {
      REQUIRE(parse("if 2>3 then if 2<3 then 4 else 6 else 7", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkIf(
          checkArithmetic(
            checkInt(2),
            checkOperation(ast::Operation::LESS_THEN, checkInt(3))
          ),
          checkInt(4),
          checkInt(6)
        ),
        checkInt(7)
      )(ast);
      REQUIRE(parse("if 2>3 then (if 2<3 then 4 else 6) else 7", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkSequence(
          checkIf(
            checkArithmetic(
              checkInt(2),
              checkOperation(ast::Operation::LESS_THEN, checkInt(3))
            ),
            checkInt(4),
            checkInt(6)
          )
        ),
        checkInt(7)
      )(ast);
      REQUIRE(parse("if 2>3 then (if 2<3 then a := 4) else b := 4", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkSequence(
          checkIf(
            checkArithmetic(
              checkInt(2),
              checkOperation(ast::Operation::LESS_THEN, checkInt(3))
            ),
            checkAssignment(
              checkLValue("a"),
              checkInt(4)
            )
          )
        ),
        checkAssignment(
          checkLValue("b"),
          checkInt(4)
        )
      )(ast);
      REQUIRE(parse("if 2>3 then (if 2<3 then a := 4 else b := 4)", ast));
      checkIf(
        checkArithmetic(
          checkInt(2),
          checkOperation(ast::Operation::GREATER_THEN, checkInt(3))
        ),
        checkSequence(
          checkIf(
            checkArithmetic(
              checkInt(2),
              checkOperation(ast::Operation::LESS_THEN, checkInt(3))
            ),
            checkAssignment(
              checkLValue("a"),
              checkInt(4)
            ),
            checkAssignment(
              checkLValue("b"),
              checkInt(4)
            )
          )
        )
      )(ast);
    }
  }
}

template<typename CheckTest, typename CheckBody>
auto checkWhile(CheckTest&& checkTest, CheckBody&& checkBody)
{
  return [=](const ast::Expression& ast)
  {
    auto pWhile = get<ast::WhileExpression>(&ast);
    REQUIRE(pWhile);
    checkTest(pWhile->test);
    checkBody(pWhile->body);
  };
}

TEST_CASE("while") {
  ast::Expression ast;
  REQUIRE(parse("while b > 0 do (a:=2;b:=b-1)", ast));
  checkWhile(
    checkArithmetic(
      checkLValue("b"),
      checkOperation(ast::Operation::GREATER_THEN, checkInt(0))
    ),
    checkSequence(
      checkAssignment(
        checkLValue("a"),
        checkInt(2)
      ),
      checkAssignment(
        checkLValue("b"),
        checkArithmetic(
          checkLValue("b"),
          checkOperation(ast::Operation::MINUS, checkInt(1))
        )
      )
    )
  )(ast);
}

template<typename CheckLo, typename CheckHi, typename CheckBody>
auto checkFor(const std::string var, CheckLo&& checkLo, CheckHi&& checkHi, CheckBody&& checkBody)
{
  return [=](const ast::Expression& ast)
  {
    auto pFor = get<ast::ForExpression>(&ast);
    checkIdentifier(var)(pFor->var);
    checkLo(pFor->lo);
    checkHi(pFor->hi);
    checkBody(pFor->body);
  };
}

TEST_CASE("for") {
  ast::Expression ast;
  REQUIRE(parse("for a := 0 to 4 do f(a)", ast));
  checkFor(
    "a",
    checkInt(0),
    checkInt(4),
    checkFunctionCall("f", checkLValue("a"))
  )(ast);
}

auto checkBreak()
{
  return [=](const ast::Expression& ast)
  {
    REQUIRE(get<ast::BreakExpression>(&ast));
  };
}

TEST_CASE("break") {
  ast::Expression ast;
  SECTION("simple") {
    REQUIRE(parse("break", ast));
    checkBreak()(ast);
  }
  SECTION("in a loop") {
    REQUIRE(parse("while b > 0 do (if(x) then break;a:=2;b:=b-1)", ast));
    checkWhile(
      checkArithmetic(
        checkLValue("b"),
        checkOperation(ast::Operation::GREATER_THEN, checkInt(0))
      ),
      checkSequence(
        checkIf(
          checkSequence(
            checkLValue("x")
          ),
          checkBreak()
        ),
        checkAssignment(
          checkLValue("a"),
          checkInt(2)
        ),
        checkAssignment(
          checkLValue("b"),
          checkArithmetic(
            checkLValue("b"),
            checkOperation(ast::Operation::MINUS, checkInt(1))
          )
        )
      )
    )(ast);
  }
}

template<typename... CheckDecls>
auto checkLet(CheckDecls&&... checkDecls)
{
  return [=](auto&&... checkExps)
  {
    return [=](const ast::Expression& ast)
    {
      auto pLet = get<ast::LetExpression>(&ast);
      REQUIRE(pLet);
      applyFunctionTupleToVector(std::forward_as_tuple(checkDecls...), pLet->decs);
      applyFunctionTupleToVector(std::forward_as_tuple(checkExps...), pLet->body);
    };
  };
}

template<typename... CheckFuncDecls>
auto checkFunctionDeclarations(CheckFuncDecls&&... checkFuncDecls)
{
  return [=](const ast::Declaration& decl)
  {
    auto pFunctionDeclarations = get<ast::FunctionDeclarations>(&decl);
    REQUIRE(pFunctionDeclarations);
    applyFunctionTupleToVector(std::forward_as_tuple(checkFuncDecls...), *pFunctionDeclarations);
  };
}

template<typename CheckInit>
auto checkVarDeclaration(const std::string& name, CheckInit&& checkInit, const boost::optional<std::string>& type = {})
{
  return [=](const ast::Declaration& decl)
  {
    auto pVarDeclaration = get<ast::VarDeclaration>(&decl);
    REQUIRE(pVarDeclaration);
    checkIdentifier(name)(pVarDeclaration->name);
    if (type) {
      REQUIRE(pVarDeclaration->type);
      checkIdentifier(*type)(*pVarDeclaration->type);
    }
    else
    {
      REQUIRE_FALSE(pVarDeclaration->type);
    }
    checkInit(pVarDeclaration->init);
  };
}

template<typename... CheckTypeDecls>
auto checkTypeDeclarations(CheckTypeDecls&&... checkTypeDecls)
{
  return [=](const ast::Declaration& decl)
  {
    auto pTypeDeclarations = get<ast::TypeDeclarations>(&decl);
    REQUIRE(pTypeDeclarations);
    applyFunctionTupleToVector(std::forward_as_tuple(checkTypeDecls...), *pTypeDeclarations);
  };
}

template<typename CheckBody, typename... CheckParams>
auto checkFunctionDeclaration(const std::string& name, const boost::optional<std::string>& result, CheckBody&& checkBody, CheckParams&&... checkParams)
{
  return [=](const ast::FunctionDeclaration& funcDecl)
  {
    checkIdentifier(name)(funcDecl.name);
    if (result)
    {
      REQUIRE(funcDecl.result);
      checkIdentifier(*result)(*funcDecl.result);
    }
    else
    {
      REQUIRE_FALSE(funcDecl.result);
    }
    applyFunctionTupleToVector(std::forward_as_tuple(checkParams...), funcDecl.params);
    checkBody(funcDecl.body);
  };
}

template<typename CheckBody, typename... CheckParams>
auto checkUntypedFunctionDeclaration(const std::string& name, CheckBody&& checkBody, CheckParams&&... checkParams)
{
  return checkFunctionDeclaration(name, boost::none, std::forward<CheckBody>(checkBody), std::forward<CheckParams>(checkParams)...);
}

template<typename CheckType>
auto checkTypeDeclaration(const std::string& name, CheckType&& checkType)
{
  return [=](const ast::TypeDeclaration& typeDecl)
  {
    checkIdentifier(name)(typeDecl.name);
    checkType(typeDecl.type);
  };
}

auto checkNameType(const std::string& name)
{
  return [=](const ast::Type& type)
  {
    auto pNameType = get<ast::NameType>(&type);
    REQUIRE(pNameType);
    checkIdentifier(name)(pNameType->name);
  };
}

template<typename... CheckFields>
auto checkRecordType(CheckFields&&... checkFields)
{
  return [=](const ast::Type& type)
  {
    auto pRecordType = get<ast::RecordType>(&type);
    REQUIRE(pRecordType);
    applyFunctionTupleToVector(std::forward_as_tuple(checkFields...), pRecordType->record);
  };
}

auto checkArrayType(const std::string& name)
{
  return [=](const ast::Type& type)
  {
    auto pArrayType = get<ast::ArrayType>(&type);
    REQUIRE(pArrayType);
    checkIdentifier(name)(pArrayType->array);
  };
}

auto checkField(const std::string& name, const std::string& type)
{
  return [=](const ast::Field& field)
  {
    checkIdentifier(name)(field.name);
    checkIdentifier(type)(field.type);
  };
}

TEST_CASE("let") {
  using namespace std::string_literals;

  ast::Expression ast;
  REQUIRE(parse(R"(
let
  /* types */
  type t1 = int
  type t2 = {a:int, b:t1}
  type t3 = array of t2
  /* vars */
  var a := 2
  var b : t3 := nil
  var c : t2 := t2{a=2}
  /* function */
  function f(i : int, b : t3) = b[2] := i
  function g() : t2 = 4
in
  f(a + g(), b);
  c.a := a
end
)", ast));
  checkLet(
    checkTypeDeclarations(
      checkTypeDeclaration("t1", checkNameType("int")),
      checkTypeDeclaration("t2", checkRecordType(checkField("a", "int"), checkField("b", "t1"))),
      checkTypeDeclaration("t3", checkArrayType("t2"))
    ),
    checkVarDeclaration("a", checkInt(2)),
    checkVarDeclaration("b", checkNil(), "t3"s),
    checkVarDeclaration("c", checkRecord("t2", checkRecordField("a", checkInt(2))), "t2"s),
    checkFunctionDeclarations(
      checkUntypedFunctionDeclaration(
        "f", 
        checkAssignment(
          checkLValue("b", checkVarSubscript(checkInt(2))),
          checkLValue("i")
        ),
        checkField("i", "int"),
        checkField("b", "t3")
      ),
      checkFunctionDeclaration(
        "g",
        "t2"s,
        checkInt(4)
      )
    )
  )
  (
    checkFunctionCall(
      "f",
      checkArithmetic(
        checkLValue("a"),
        checkOperation(ast::Operation::PLUS, checkFunctionCall("g"))
      ),
      checkLValue("b")
    ),
    checkAssignment(
      checkLValue("c", checkVarField("a")),
      checkLValue("a")
    )
  )(ast);
}