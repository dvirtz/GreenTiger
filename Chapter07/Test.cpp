#include "Program.h"
#include "testsHelper.h"
#include "vectorApply.h"
#include "x64FastCall/Registers.h"
#include "x64FastCall/Frame.h"
#define CATCH_CONFIG_MAIN
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/variant/get.hpp>
#include <catch.hpp>

using namespace tiger;
using OptReg = boost::optional<temp::Register>;
using OptLabel = boost::optional<temp::Label>;

TEST_CASE("compile test files") {
  namespace fs = boost::filesystem;
  forEachTigerTest(
      [](const fs::path &filepath, bool parseError, bool compilationError) {
        SECTION(filepath.filename().string()) {
          if (parseError || compilationError) {
            REQUIRE_FALSE(compileFile(filepath.string()));
          } else {
            REQUIRE(compileFile(filepath.string()));
          }
        }
      });
}

using boost::get;
using helpers::applyFunctionsToVector;

enum { WORD_SIZE = tiger::frame::x64FastCall::Frame::WORD_SIZE };

template <typename T> void setOrCheck(boost::optional<T> &op, T val) {
  if (op) {
    REQUIRE(*op == val);
  } else {
    op = val;
  }
}

ir::Expression checkedCompile(const std::string &string) {
  auto res = tiger::compile(string);
  REQUIRE(res);
  return *res;
}

template <typename CheckStatement, typename CheckExpression>
auto checkExpressionSequence(CheckStatement &&checkStatement,
                             CheckExpression &&checkExpression) {
  return [
    checkStatement = std::forward<CheckStatement>(checkStatement),
    checkExpression = std::forward<CheckExpression>(checkExpression)
  ](const ir::Expression &exp) {
    auto seq = get<ir::ExpressionSequence>(&exp);
    REQUIRE(seq);
    checkStatement(seq->stm);
    checkExpression(seq->exp);
  };
}

template <typename... CheckStatements>
auto checkSequence(CheckStatements &&... checkStatements) {
  return [
    size = sizeof...(checkStatements),
    checkStatements =
        std::forward_as_tuple(std::forward<CheckStatements>(checkStatements)...)
  ](const ir::Statement &stm) {
    auto seq = get<ir::Sequence>(&stm);
    REQUIRE(seq);
    REQUIRE(seq->statements.size() == size);
    applyFunctionTupleToVector(seq->statements, checkStatements);
  };
}

template <typename CheckDest, typename CheckSrc>
auto checkMove(CheckDest &&checkDest, CheckSrc &&checkSrc) {
  return [
    checkDest = std::forward<CheckDest>(checkDest),
    checkSrc = std::forward<CheckSrc>(checkSrc)
  ](const ir::Statement &stm) {
    auto move = get<ir::Move>(&stm);
    REQUIRE(move);
    checkDest(move->dst);
    checkSrc(move->src);
  };
}

auto checkLabel(OptLabel &label) {
  return [&label](const auto &exp) {
    auto pLabel = get<temp::Label>(&exp);
    REQUIRE(pLabel);
    setOrCheck(label, *pLabel);
  };
}

auto checkReg(OptReg &reg) {
  return [&reg](const ir::Expression &exp) {
    auto pReg = get<temp::Register>(&exp);
    REQUIRE(pReg);
    setOrCheck(reg, *pReg);
  };
}

auto checkInt(int i) {
  return [i](const ir::Expression &exp) {
    auto pInt = get<int>(&exp);
    REQUIRE(pInt);
    REQUIRE(*pInt == i);
  };
}

template <typename... CheckArgs>
auto checkCall(OptLabel &label, const std::tuple<CheckArgs...> &checkArgs) {
  return [
    &label, &checkArgs,
    size = std::tuple_size<std::decay_t<decltype(checkArgs)>>::value
  ](const ir::Expression &exp) {
    auto call = get<ir::Call>(&exp);
    REQUIRE(call);
    checkLabel(label)(call->fun);
    REQUIRE(call->args.size() == size);
    applyFunctionTupleToVector(call->args, checkArgs);
  };
}

template <typename CheckLeft, typename CheckRight>
auto checkBinaryOperation(ir::BinOp op, CheckLeft &&checkLeft,
                          CheckRight &&checkRight) {
  return [
    op, checkLeft = std::forward<CheckLeft>(checkLeft),
    checkRight = std::forward<CheckRight>(checkRight)
  ](const ir::Expression &exp) {
    auto binOperation = get<ir::BinaryOperation>(&exp);
    REQUIRE(binOperation);
    REQUIRE(binOperation->op == op);
    checkLeft(binOperation->left);
    checkRight(binOperation->right);
  };
}

template <typename... CheckArgs>
auto checkLocalCall(OptLabel &label, CheckArgs &&... checkArgs) {
  return [&label, checkArgs = std::forward_as_tuple(checkArgs...) ](
      const ir::Expression &exp) {
    using namespace tiger::frame::x64FastCall;
    OptReg fp = reg(Registers::RBP);
    return checkCall(label, std::tuple_cat(std::make_tuple(checkBinaryOperation(
                                               ir::BinOp::PLUS, checkReg(fp),
                                               checkInt(0 - WORD_SIZE))),
                                           checkArgs));
  };
}

template <typename... CheckArgs>
auto checkLibraryCall(const std::string &name, CheckArgs &&... checkArgs) {
  return [
    name,
    checkArgs = std::forward_as_tuple(std::forward<CheckArgs>(checkArgs)...)
  ](const ir::Expression &args) {
    OptLabel label = temp::Label{name};
    return checkCall(label, checkArgs);
  };
}

template <typename CheckExp>
auto checkExpressionStatement(CheckExp &&checkExp) {
  return [checkExp =
              std::forward<CheckExp>(checkExp)](const ir::Statement &stm) {
    auto expStatement = get<ir::ExpressionStatement>(&stm);
    REQUIRE(expStatement);
    checkExp(expStatement->exp);
  };
}

// a nop is implemented as 0
auto checkNop() { return checkExpressionStatement(checkInt(0)); }

template <typename CheckLeft, typename CheckRight>
auto checkConditionalJump(ir::RelOp relOp, CheckLeft &&checkLeft,
                          CheckRight &&checkRight, OptLabel &trueDest,
                          OptLabel *falseDest = nullptr) {
  return [
    relOp, checkLeft = std::forward<CheckLeft>(checkLeft),
    checkRight = std::forward<CheckRight>(checkRight), &trueDest, falseDest
  ](const ir::Statement &stm) {
    auto pCondJump = get<ir::ConditionalJump>(&stm);
    REQUIRE(pCondJump);
    REQUIRE(pCondJump->op == relOp);
    checkLeft(pCondJump->left);
    checkRight(pCondJump->right);
    REQUIRE(pCondJump->trueDest);
    setOrCheck(trueDest, *pCondJump->trueDest);
    if (falseDest) {
      REQUIRE(pCondJump->falseDest);
      setOrCheck(*falseDest, *pCondJump->falseDest);
    } else {
      REQUIRE_FALSE(pCondJump->falseDest);
    }
  };
}

auto checkString(OptLabel &stringLabel) {
  // a string is translated to the label of the string address
  return checkLabel(stringLabel);
}

template <typename CheckAddress>
auto checkMemoryAccess(CheckAddress &&checkAddress) {
  return [checkAddress = std::forward<CheckAddress>(checkAddress)](
      const ir::Expression &exp) {
    auto pMemAccess = get<ir::MemoryAccess>(&exp);
    checkAddress(pMemAccess->address);
  };
}

template <typename CheckExp> auto checkJump(CheckExp &&checkExp) {
  return [checkExp =
              std::forward<CheckExp>(checkExp)](const ir::Statement &stm) {
    auto pJump = get<ir::Jump>(&stm);
    REQUIRE(pJump);
    checkExp(pJump->exp);
  };
}

template <typename CheckStm>
auto checkStatementExpression(CheckStm &&checkStm) {
  return checkExpressionSequence(std::forward<CheckStm>(checkStm), checkInt(0));
}

auto checkMemberAddress(OptReg &r, int index) {
  return checkMemoryAccess(
      checkBinaryOperation(ir::BinOp::PLUS, checkReg(r),
                           checkBinaryOperation(ir::BinOp::MUL, checkInt(index),
                                                checkInt(WORD_SIZE))));
}

TEST_CASE("lvalue") {
  SECTION("identifier") {
    auto exp = checkedCompile(R"(
let
  var i := 0
in
  i
end
)");
    OptReg reg;
    checkExpressionSequence(
        checkSequence( // declarations
            checkMove( // allocates a register and set it to 0
                checkReg(reg), checkInt(0))),
        checkReg(reg) // return previously allocated reg
        )(exp);
  }
  SECTION("field") {
    auto exp = checkedCompile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.i
end
)");
    OptReg reg;
    checkExpressionSequence(
        checkSequence(  // declarations
            checkNop(), // type declarations are no-ops
            checkMove(  // allocates a register and set it to 0
                checkReg(reg), checkInt(0))),
        checkBinaryOperation( // returns the offset of i in rec relative to
                              // previously allocated reg
            ir::BinOp::PLUS, checkReg(reg),
            checkBinaryOperation(ir::BinOp::MUL, checkInt(0),
                                 checkInt(WORD_SIZE))))(exp);
  }
  SECTION("array element") {
    auto exp = checkedCompile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[1]
end
)");
    OptReg reg1, reg2;
    checkExpressionSequence(
        checkSequence(  // declarations
            checkNop(), // type declarations are no-ops,
            checkMove(
                checkReg(reg1), // move reg2 to reg1,
                checkExpressionSequence(
                    checkSequence(
                        checkMove( // set reg2 to result of malloc(array size),
                            checkReg(reg2),
                            checkLibraryCall("malloc", checkBinaryOperation(
                                                           ir::BinOp::MUL,
                                                           checkInt(WORD_SIZE),
                                                           checkInt(2)))),
                        checkExpressionStatement(
                            checkLibraryCall( // call initArray(array size,
                                              // array init val)
                                "initArray", checkInt(2), checkInt(3)))),
                    checkReg(reg2)))),
        // returns the offset of element 1 in arr relative
        // to reg1
        checkBinaryOperation(ir::BinOp::PLUS, checkReg(reg1),
                             checkBinaryOperation(ir::BinOp::MUL, checkInt(1),
                                                  checkInt(WORD_SIZE))))(exp);
  }
}

TEST_CASE("nil") {
  auto exp = checkedCompile("nil");
  checkInt(0) // same as 0
      (exp);
}

TEST_CASE("sequence") {
  // the last expression is the sequence results, the previous ones are just
  // statements
  SECTION("empty") {
    auto exp = checkedCompile("()");
    checkInt(0)(exp);
  }

  SECTION("single") {
    auto exp = checkedCompile("(42)");
    checkInt(42)(exp);
  }

  SECTION("multiple") {
    auto exp = checkedCompile(R"((1;"two";flush()))");
    OptLabel stringLabel;
    checkExpressionSequence(
        checkSequence(checkExpressionStatement(checkInt(1)),
                      checkExpressionStatement(checkString(stringLabel))),
        checkLibraryCall("flush"))(exp);
  }
}

TEST_CASE("integer") {
  auto exp = checkedCompile("42");
  checkInt(42)(exp);
}

TEST_CASE("string") {
  auto exp = checkedCompile(R"("\tHello \"World\"!\n")");
  OptLabel stringLabel;
  checkString(stringLabel)(exp);
}

TEST_CASE("function call") {
  using namespace tiger::frame::x64FastCall;
  OptReg rv = reg(Registers::RAX);
  OptLabel functionLabel;

  SECTION("with no arguments") {
    auto exp = checkedCompile(R"(
let
  function f() = ()
in
  f()
end
)");
    checkExpressionSequence(
        checkSequence(                         // declarations
            checkSequence(                     // functions declaration
                checkSequence(                 // f
                    checkLabel(functionLabel), // function label
                    checkMove(                 // body
                        checkReg(rv), checkInt(0))))),
        checkLocalCall(functionLabel) // call
        )(exp);
  }

  SECTION("with arguments") {
    auto exp = checkedCompile(R"(
let
  function f(a: int) = ()
in
  f(2)
end
)");
    checkExpressionSequence(
        checkSequence(                         // declarations
            checkSequence(                     // functions declaration
                checkSequence(                 // f
                    checkLabel(functionLabel), // function label
                    checkMove(                 // body
                        checkReg(rv), checkInt(0))))),
        checkLocalCall(functionLabel) // call
        )(exp);
  }
}

TEST_CASE("record") {
  SECTION("empty") {
    auto exp = checkedCompile(R"(
let 
  type t = {}
in
  t{}
end
)");
    OptReg r;
    checkExpressionSequence(
        checkSequence( // declarations
            checkNop() // type declaration
            ),
        checkExpressionSequence(
            checkSequence(checkMove( // move result of malloc(record_size) to r
                checkReg(r), checkLibraryCall("malloc", checkInt(0)))),
            checkReg(r)))(exp);
  }

  SECTION("not empty") {
    auto exp = checkedCompile(R"(
let 
  type t = {a: int, b: string}
in
  t{a = 2, b = "hello"}
end
)");

    OptReg r;
    OptLabel stringLabel;
    checkExpressionSequence(
        checkSequence( // declarations
            checkNop() // type declaration
            ),
        checkExpressionSequence(
            checkSequence(
                checkMove( // move result of malloc(record_size) to r
                    checkReg(r), checkLibraryCall("malloc", checkInt(8))),
                checkMove( // init first member with 2
                    checkMemberAddress(r, 0), checkInt(2)),
                checkMove( // init second member with "hello"
                    checkMemberAddress(r, 1), checkString(stringLabel))),
            checkReg(r)))(exp);
  }
}

TEST_CASE("array") {
  auto exp = checkedCompile(R"(
let
  type t = array of int
in
  t[2] of 3
end
)");
  OptReg r;
  checkExpressionSequence(
      checkSequence( // declarations
          checkNop() // type declaration
          ),
      checkExpressionSequence(
          checkSequence(
              checkMove( // move result of malloc(array_size) to r
                  checkReg(r),
                  checkLibraryCall("malloc",
                                   checkBinaryOperation(ir::BinOp::MUL,
                                                        checkInt(WORD_SIZE),
                                                        checkInt(2)))),
              checkExpressionStatement( // call initArray(array_size, init_val)
                  checkLibraryCall("initArray", checkInt(2), checkInt(3)))),
          checkReg(r)))(exp);
}

TEST_CASE("arithmetic") {
  static const std::pair<std::string, ir::BinOp> operations[] = {
      {"+", ir::BinOp::PLUS},
      {"-", ir::BinOp::MINUS},
      {"*", ir::BinOp::MUL},
      {"/", ir::BinOp::DIV}};

  for (const auto &operation : operations) {
    SECTION(Catch::StringMaker<ir::BinOp>::convert(operation.second)) {
      auto exp = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
    i
  end
  )") % operation.first));
      OptReg r;
      checkExpressionSequence(
          checkSequence(             // declarations
              checkMove(checkReg(r), // move result of 2 op 3 to a r
                        checkBinaryOperation(operation.second, checkInt(2),
                                             checkInt(3)))),
          checkReg(r) // return i
          )(exp);
    }
  }
}

TEST_CASE("comparison") {
  static const std::pair<std::string, ir::RelOp> relations[] = {
      {"=", ir::RelOp::EQ},  {"<>", ir::RelOp::NE}, {"<", ir::RelOp::LT},
      {"<=", ir::RelOp::LE}, {">", ir::RelOp::GT},  {">=", ir::RelOp::GE},
      {"=", ir::RelOp::EQ}};
  SECTION("integer") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto exp = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
    i
  end
  )") % relation.first));
        OptReg i, r;
        OptLabel trueDest, falseDest;
        checkExpressionSequence(
            checkSequence( // declarations
                checkMove(
                    checkReg(i),
                    checkExpressionSequence(
                        checkSequence(
                            checkMove(checkReg(r),
                                      checkInt(1)), // move 1 to r
                            checkConditionalJump(
                                relation.second, checkInt(2), checkInt(3),
                                trueDest, &falseDest), // check condition,
                                                       // jumping to trueDest
                                                       // or falseDest
                                                       // accordingly
                            checkLabel(falseDest),
                            checkMove(
                                checkReg(r),
                                checkInt(
                                    0)), // if condition is false, move 0 to r
                            checkLabel(trueDest)),
                        checkReg(r) // move r to i
                        ))),
            checkReg(i) // return i
            )(exp);
      }
    }
  }

  SECTION("string") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto exp = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := "2" %1% "3"
  in
    i
  end
  )") % relation.first));
        OptReg r1, r2;
        OptLabel trueDest, falseDest;
        OptLabel leftString, rightString;
        checkExpressionSequence(
            checkSequence( // declarations
                checkMove(
                    checkReg(r1),
                    checkExpressionSequence(
                        checkSequence(
                            checkMove(checkReg(r2),
                                      checkInt(1)), // move 1 to r2
                            checkConditionalJump(   // stringCompare(s1, s2)
                                                  // returns -1, 0 or 1 when s1
                                                  // is less than, equal or
                                                  // greater than s2,
                                                  // respectively so s1 op s2 is
                                                  // the same as
                                                  // stringCompare(s1, s2) op 0
                                relation.second,
                                checkLibraryCall("stringCompare",
                                                 checkString(leftString),
                                                 checkString(rightString)),
                                checkInt(0), trueDest,
                                &falseDest), // check condition,
                                             // jumping to trueDest
                                             // or falseDest
                                             // accordingly
                            checkLabel(falseDest),
                            checkMove(
                                checkReg(r2),
                                checkInt(
                                    0)), // if condition is false, move 0 to r2
                            checkLabel(trueDest)),
                        checkReg(r2) // move r2 to r1
                        ))),
            checkReg(r1) // return i
            )(exp);
      }
    }
  }

  static const std::pair<std::string, ir::RelOp> eqNeq[] = {
      {"=", ir::RelOp::EQ}, {"<>", ir::RelOp::NE}};

  SECTION("record") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto exp = checkedCompile(boost::str(boost::format(R"(
let
  type t = {}
  var i := t{}
in
  i %1% i
end
)") % relation.first));
        OptReg i, r1, r2;
        OptLabel trueDest, falseDest;
        checkExpressionSequence(
            checkSequence(  // declarations
                checkNop(), // type declaration
                checkMove(  // move record to i
                    checkReg(i),
                    checkExpressionSequence(
                        checkSequence(checkMove( // move result of
                                                 // malloc(record_size) to r
                            checkReg(r1),
                            checkLibraryCall("malloc", checkInt(0)))),
                        checkReg(r1)))),
            checkExpressionSequence(
                checkSequence(
                    checkMove(checkReg(r2),
                              checkInt(1)), // move 1 to r2
                    checkConditionalJump(relation.second, checkReg(i),
                                         checkReg(i), trueDest,
                                         &falseDest), // check condition,
                                                      // jumping to trueDest
                                                      // or falseDest
                                                      // accordingly
                    checkLabel(falseDest),
                    checkMove(
                        checkReg(r2),
                        checkInt(0)), // if condition is false, move 0 to r2
                    checkLabel(trueDest)),
                checkReg(r2) // move r2 to r1
                ))(exp);
      }
    }
  }

  SECTION("array") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto exp = checkedCompile(boost::str(boost::format(R"(
let
  type t = array of int
  var i := t[2] of 3
in
  i %1% i
end
)") % relation.first));
        OptReg i, r1, r2;
        OptLabel trueDest, falseDest;
        checkExpressionSequence(
            checkSequence(  // declarations
                checkNop(), // type declaration
                checkMove(  // move array to i
                    checkReg(i),
                    checkExpressionSequence(
                        checkSequence(
                            checkMove( // move result of malloc(array_size) to
                                       // r1
                                checkReg(r1),
                                checkLibraryCall(
                                    "malloc",
                                    checkBinaryOperation(ir::BinOp::MUL,
                                                         checkInt(WORD_SIZE),
                                                         checkInt(2)))),
                            checkExpressionStatement( // call
                                                      // initArray(array_size,
                                                      // init_val)
                                checkLibraryCall("initArray", checkInt(2),
                                                 checkInt(3)))),
                        checkReg(r1)))),
            checkExpressionSequence(
                checkSequence(
                    checkMove(checkReg(r2),
                              checkInt(1)), // move 1 to r2
                    checkConditionalJump(relation.second, checkReg(i),
                                         checkReg(i), trueDest,
                                         &falseDest), // check condition,
                                                      // jumping to trueDest
                                                      // or falseDest
                                                      // accordingly
                    checkLabel(falseDest),
                    checkMove(
                        checkReg(r2),
                        checkInt(0)), // if condition is false, move 0 to r2
                    checkLabel(trueDest)),
                checkReg(r2) // move r2 to r1
                ))(exp);
      }
    }
  }
}

TEST_CASE("boolean") {
  SECTION("and") {
    auto exp = checkedCompile("2 & 3");
    // a & b is translated to if a then b else 0
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkExpressionSequence(
        checkSequence(
            checkConditionalJump( // test
                ir::RelOp::NE, checkInt(2), checkInt(0), trueDest, &falseDest),
            checkSequence( // then
                checkLabel(trueDest), checkMove(checkReg(r), checkInt(3)), // 3
                checkJump(checkLabel(joinDest))),
            checkSequence( // else
                checkLabel(falseDest), checkMove(checkReg(r), checkInt(0)), // 0
                checkJump(checkLabel(joinDest))),
            checkLabel(joinDest) // branches join here
            ),
        checkReg(r))(exp);
  }

  SECTION("or") {
    auto exp = checkedCompile("2 | 3");
    // a | b is translated to if a then 1 else b
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkExpressionSequence(
        checkSequence(
            checkConditionalJump( // test
                ir::RelOp::NE, checkInt(2), checkInt(0), trueDest, &falseDest),
            checkSequence( // then
                checkLabel(trueDest), checkMove(checkReg(r), checkInt(1)), // 1
                checkJump(checkLabel(joinDest))),
            checkSequence( // else
                checkLabel(falseDest), checkMove(checkReg(r), checkInt(3)), // 3
                checkJump(checkLabel(joinDest))),
            checkLabel(joinDest) // branches join here
            ),
        checkReg(r))(exp);
  }
}

TEST_CASE("assignment") {
  auto exp = checkedCompile(R"(
let
  var a := 3
  var b := 0
in
  b := a
end
)");
  OptReg a, b;
  checkExpressionSequence(checkSequence( // declarations
                              checkMove( // move 3 to a
                                  checkReg(a), checkInt(3)),
                              checkMove( // move 0 to b
                                  checkReg(b), checkInt(0))),
                          checkStatementExpression(checkMove( // move b to a
                              checkReg(b), checkReg(a))))(exp);
}

TEST_CASE("if then") {
  SECTION("no else") {
    auto exp = checkedCompile(R"(
let
  var a := 0
in
  if 1 then a := 3
end
)");
    OptReg a;
    OptLabel trueDest, falseDest, joinDest;
    checkExpressionSequence(
        checkSequence( // declarations
            checkMove( // move 0 to a
                checkReg(a), checkInt(0))),
        checkStatementExpression(checkSequence(
            checkConditionalJump( // test
                ir::RelOp::NE, checkInt(1), checkInt(0), trueDest, &falseDest),
            checkSequence( // then
                checkLabel(trueDest),
                checkMove(checkReg(a),
                          checkInt(3)), // move 3 to a
                checkJump(checkLabel(joinDest))),
            checkLabel(joinDest) // branches join here
            )))(exp);
  }

  SECTION("with else") {
    SECTION("both expression are of type void") {
      auto exp = checkedCompile(R"(
let
  var a := 0
in
  if 0 then a := 3 else a := 4
end
)");
      OptReg a;
      OptLabel trueDest, falseDest, joinDest;
      checkExpressionSequence(checkSequence( // declarations
                                  checkMove( // move 0 to a
                                      checkReg(a), checkInt(0))),
                              checkStatementExpression(checkSequence(
                                  checkConditionalJump( // test
                                      ir::RelOp::NE, checkInt(0), checkInt(0),
                                      trueDest, &falseDest),
                                  checkSequence( // then
                                      checkLabel(trueDest),
                                      checkMove(checkReg(a),
                                                checkInt(3)), // move 3 to a
                                      checkJump(checkLabel(joinDest))),
                                  checkSequence( // else
                                      checkLabel(falseDest),
                                      checkMove(checkReg(a),
                                                checkInt(4)), // move 4 to a
                                      checkJump(checkLabel(joinDest))),
                                  checkLabel(joinDest) // branches join here
                                  )))(exp);
    }

    SECTION("both expression are of the same type") {
      auto exp = checkedCompile(R"(
let
  var a := if 1 then 2 else 3
in
  a
end
)");
      OptReg a, r;
      OptLabel trueDest, falseDest, joinDest;
      checkExpressionSequence(
          checkSequence( // declarations
              checkMove(
                  checkReg(a),
                  checkExpressionSequence(
                      checkSequence(checkConditionalJump( // test
                                        ir::RelOp::NE, checkInt(1), checkInt(0),
                                        trueDest, &falseDest),
                                    checkSequence( // then
                                        checkLabel(trueDest),
                                        checkMove(checkReg(r),
                                                  checkInt(2)), // move 2 to r
                                        checkJump(checkLabel(joinDest))),
                                    checkSequence( // else
                                        checkLabel(falseDest),
                                        checkMove(checkReg(r),
                                                  checkInt(3)), // move 3 to r
                                        checkJump(checkLabel(joinDest))),
                                    checkLabel(joinDest) // branches join here
                                    ),
                      checkReg(r)))), // move r to a
          checkReg(a)                 // return a
          )(exp);
    }
  }
}

TEST_CASE("while") {
  auto exp = checkedCompile(R"(
let
  var b := 2
  var a := 0
in
  while b > 0 do (a:=a+1)
end
)");
  OptReg a, b, r;
  OptLabel loopStart, loopDone, trueDest, falseDest;
  checkExpressionSequence(
      checkSequence( // declarations
          checkMove( // move 2 to b
              checkReg(b), checkInt(2)),
          checkMove( // move 0 to a
              checkReg(a), checkInt(0))),
      checkStatementExpression(checkSequence( // while
          checkLabel(loopStart),
          checkConditionalJump( // test
              ir::RelOp::EQ,
              checkExpressionSequence( // b > 0
                  checkSequence(
                      checkMove(checkReg(r),
                                checkInt(1)), // move 1 to r
                      checkConditionalJump(ir::RelOp::GT, checkReg(b),
                                           checkInt(0), trueDest,
                                           &falseDest), // check condition,
                                                        // jumping to trueDest
                                                        // or falseDest
                                                        // accordingly
                      checkLabel(falseDest),
                      checkMove(
                          checkReg(r),
                          checkInt(0)), // if condition is false, move 0 to r
                      checkLabel(trueDest)),
                  checkReg(r)),
              checkInt(0),
              loopDone // if test equals 0 jumps to loopDone
              ),
          checkMove( // a := a + 1
              checkReg(a),
              checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
          checkJump( // jump back to loop Start
              checkLabel(loopStart)),
          checkLabel(loopDone))))(exp);
}

TEST_CASE("for") {
  auto exp = checkedCompile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)");
  OptReg rv, a, limit;
  OptLabel functionLabel, loopStart, loopDone;
  checkExpressionSequence(
      checkSequence(                         // declarations
          checkSequence(                     // functions declaration
              checkSequence(                 // f
                  checkLabel(functionLabel), // function label
                  checkMove(                 // body
                      checkReg(rv), checkInt(0))))),
      checkStatementExpression(checkSequence( // for
          checkMove(                          // move 0 to a
              checkReg(a), checkInt(0)),
          checkMove( // move 4 to limit
              checkReg(limit), checkInt(4)),
          checkConditionalJump( // skip the loop if a > limit
              ir::RelOp::GT, checkReg(a), checkReg(limit), loopDone),
          checkLabel(loopStart),
          checkExpressionStatement( // body
              checkLocalCall(       // f(a)
                  functionLabel, checkReg(a))),
          checkMove( // a := a + 1
              checkReg(a),
              checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
          checkConditionalJump(ir::RelOp::LT, checkReg(a), checkReg(limit),
                               loopStart),
          checkLabel(loopDone))))(exp);
}

TEST_CASE("break") {
  SECTION("for") {
    auto exp = checkedCompile("for a := 0 to 2 do break");
    OptReg a, limit;
    OptLabel functionLabel, loopStart, loopDone;
    checkStatementExpression(checkSequence( // for
        checkMove(                          // move 0 to a
            checkReg(a), checkInt(0)),
        checkMove( // move 4 to limit
            checkReg(limit), checkInt(2)),
        checkConditionalJump( // skip the loop if a > limit
            ir::RelOp::GT, checkReg(a), checkReg(limit), loopDone),
        checkLabel(loopStart),
        checkJump( // break
            checkLabel(loopDone)),
        checkMove( // a := a + 1
            checkReg(a),
            checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
        checkConditionalJump(ir::RelOp::LT, checkReg(a), checkReg(limit),
                             loopStart),
        checkLabel(loopDone)))(exp);
  }

  SECTION("while") {
    auto exp = checkedCompile("while 1 do break");
    OptReg a, b, r;
    OptLabel loopStart, loopDone, trueDest, falseDest;
    checkStatementExpression(checkSequence( // while
        checkLabel(loopStart),
        checkConditionalJump( // test
            ir::RelOp::EQ, checkInt(1), checkInt(0),
            loopDone // if test equals 0 jumps to loopDone
            ),
        checkJump( // break
            checkLabel(loopDone)),
        checkJump( // jump back to loop Start
            checkLabel(loopStart)),
        checkLabel(loopDone)))(exp);
  }
}

TEST_CASE("type declarations") {
  SECTION("name") {
    auto exp = checkedCompile(R"(
let
  type a = int
  var b : a := 0
in
end
)");
    OptReg b;
    checkStatementExpression(checkSequence( // declarations
        checkNop(),                         // type declarations are no-ops
        checkMove(                          // move 0 to b
            checkReg(b), checkInt(0))))(exp);
  }

  SECTION("record") {
    auto exp = checkedCompile(R"(
let
  type rec = {i : int}
  var r := rec{i=2}
in
end
)");
    OptReg r1, r2;
    checkStatementExpression(checkSequence( // declarations
        checkNop(),                         // type declarations are no-ops
        checkMove(                          // move r2 to r1
            checkReg(r1),
            checkExpressionSequence(
                checkSequence(
                    checkMove( // move result of malloc(record_size) to r2
                        checkReg(r2), checkLibraryCall("malloc", checkInt(4))),
                    checkMove( // init first member with 2
                        checkMemberAddress(r2, 0), checkInt(2))),
                checkReg(r2)))))(exp);
  }

  SECTION("array") {
    auto exp = checkedCompile(R"(
let
  type arr = array of string
  var a := arr[3] of "a"
in
end
)");
    OptReg r1, r2;
    OptLabel stringLabel;
    checkStatementExpression(checkSequence( // declarations
        checkNop(),                         // type declarations are no-ops,
        checkMove(
            checkReg(r1), // move reg2 to reg1,
            checkExpressionSequence(
                checkSequence(
                    checkMove( // set reg2 to result of malloc(array size),
                        checkReg(r2),
                        checkLibraryCall(
                            "malloc", checkBinaryOperation(ir::BinOp::MUL,
                                                           checkInt(WORD_SIZE),
                                                           checkInt(3)))),
                    checkExpressionStatement(
                        checkLibraryCall( // call initArray(array size,
                                          // array init val)
                            "initArray", checkInt(3),
                            checkString(stringLabel)))),
                checkReg(r2)))))(exp);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto exp = checkedCompile(R"(
let
  type intlist = {hd: int, tl: intlist}
in
  intlist{hd = 3, tl = intlist{hd = 4, tl = nil}}
end
)");
      OptReg r1, r2;
      checkExpressionSequence(
          checkSequence( // declarations
              checkNop() // type declaration
              ),
          checkExpressionSequence( // outer record
              checkSequence(
                  checkMove( // move result of malloc(record_size) to r1
                      checkReg(r1), checkLibraryCall("malloc", checkInt(8))),
                  checkMove( // init first member
                      checkMemberAddress(r1, 0), checkInt(3)),
                  checkMove( // init second member
                      checkMemberAddress(r1, 1),
                      checkExpressionSequence( // inner record
                          checkSequence(
                              checkMove( // move result of malloc(record_size)
                                         // to r2
                                  checkReg(r2),
                                  checkLibraryCall("malloc", checkInt(8))),
                              checkMove( // init first member
                                  checkMemberAddress(r2, 0), checkInt(4)),
                              checkMove( // init second member
                                  checkMemberAddress(r2, 1), checkInt(0))),
                          checkReg(r2)))),
              checkReg(r1)))(exp);
    }

    SECTION("mutually") {
      auto exp = checkedCompile(R"(
let
  type tree = {key: int, children: treelist}
  type treelist = {hd: tree, tl: treelist}
in
  tree{key = 1, children = treelist{hd = tree{key = 2, children = nil}, tl = nil}}
end
)");
      OptReg r1, r2, r3;
      checkExpressionSequence(
          checkSequence( // declarations
              checkNop() // type declaration
              ),
          checkExpressionSequence( // outer record
              checkSequence(
                  checkMove( // move result of malloc(record_size) to r1
                      checkReg(r1), checkLibraryCall("malloc", checkInt(8))),
                  checkMove( // init first member
                      checkMemberAddress(r1, 0), checkInt(1)),
                  checkMove( // init second member
                      checkMemberAddress(r1, 1),
                      checkExpressionSequence( // inner record
                          checkSequence(
                              checkMove( // move result of malloc(record_size)
                                         // to r2
                                  checkReg(r2),
                                  checkLibraryCall("malloc", checkInt(8))),
                              checkMove( // init first member
                                  checkMemberAddress(r2, 0),
                                  checkExpressionSequence( // most inner record
                                      checkSequence(
                                          checkMove( // move result of
                                                     // malloc(record_size) to
                                                     // r3
                                              checkReg(r3),
                                              checkLibraryCall("malloc",
                                                               checkInt(8))),
                                          checkMove( // init first member
                                              checkMemberAddress(r3, 0),
                                              checkInt(2)),
                                          checkMove( // init second member
                                              checkMemberAddress(r3, 1),
                                              checkInt(0))),
                                      checkReg(r3))),
                              checkMove( // init second member
                                  checkMemberAddress(r2, 1), checkInt(0))),
                          checkReg(r2)))),
              checkReg(r1)))(exp);
    }
  }
}

TEST_CASE("function declarations") {
  using namespace tiger::frame::x64FastCall;

  OptReg rv = reg(Registers::RAX);

  SECTION("simple") {
    auto exp = checkedCompile(R"(
let
  function f() = ()
in
end
)");
    OptLabel functionLabel;
    checkStatementExpression(checkSequence( // declarations
        checkSequence(                      // functions declaration
            checkSequence(                  // f
                checkLabel(functionLabel),  // function label
                checkMove(                  // body
                    checkReg(rv), checkInt(0))))))(exp);
  }

  SECTION("with parameters") {
    auto exp = checkedCompile(R"(
let
  function f(a:int, b:string) = ()
in
end
)");
    OptLabel functionLabel;
    checkStatementExpression(checkSequence( // declarations
        checkSequence(                      // functions declaration
            checkSequence(                  // f
                checkLabel(functionLabel),  // function label
                checkMove(                  // body
                    checkReg(rv), checkInt(0))))))(exp);
  }

  SECTION("with return value") {
    auto exp = checkedCompile(R"(
let
  function f(a:int, b:string) : string = (a:=1;b)
in
end
)");
    OptReg a, b;
    OptLabel functionLabel;
    checkStatementExpression(checkSequence( // declarations
        checkSequence(                      // functions declaration
            checkSequence(                  // f
                checkLabel(functionLabel),  // function label
                checkMove(                  // body
                    checkReg(rv),
                    checkExpressionSequence(checkSequence(checkMove( // a=1
                                                checkReg(a), checkInt(1))),
                                            checkReg(b)))))))(exp);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto exp = checkedCompile(R"(
let
  function fib(n:int) : int = (
    if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
  )
in
end
)");
      CAPTURE(exp);
      OptReg fp = reg(Registers::RBP), n, r[3];
      OptLabel functionLabel, trueDest[3], falseDest[3], joinDest[3];
      checkStatementExpression(checkSequence( // declarations
          checkSequence(                      // functions declaration
              checkSequence(                  // f
                  checkLabel(functionLabel),  // function label
                  checkMove(                  // body
                      checkReg(rv),
                      checkExpressionSequence(
                          checkSequence(
                              checkConditionalJump( // test
                                  ir::RelOp::NE,
                                  checkExpressionSequence(
                                      checkSequence(
                                          checkConditionalJump( // n = 0
                                              ir::RelOp::EQ, checkReg(n),
                                              checkInt(0), trueDest[0],
                                              &falseDest[0]),
                                          checkSequence( // then
                                              checkLabel(trueDest[0]),
                                              checkMove(checkReg(r[0]),
                                                        checkInt(1)),
                                              checkJump(
                                                  checkLabel(joinDest[0]))),
                                          checkSequence( // else
                                              checkLabel(falseDest[0]),
                                              checkMove(
                                                  checkReg(r[0]),
                                                  checkExpressionSequence( // n
                                                                           // =
                                                                           // 1
                                                      checkSequence(
                                                          checkMove(
                                                              checkReg(r[1]),
                                                              checkInt(1)),
                                                          checkConditionalJump(
                                                              ir::RelOp::EQ,
                                                              checkReg(n),
                                                              checkInt(1),
                                                              trueDest[1],
                                                              &falseDest[1]),
                                                          checkLabel(
                                                              falseDest[1]),
                                                          checkMove(
                                                              checkReg(r[1]),
                                                              checkInt(0)),
                                                          checkLabel(
                                                              trueDest[1])),
                                                      checkReg(r[1]))),
                                              checkJump(
                                                  checkLabel(joinDest[0]))),
                                          checkLabel(joinDest[0])),
                                      checkReg(r[0])),
                                  checkInt(0), trueDest[2], &falseDest[2]),
                              checkSequence( // then
                                  checkLabel(trueDest[2]),
                                  checkMove(checkReg(r[2]), checkReg(n)),
                                  checkJump(checkLabel(joinDest[2]))),
                              checkSequence( // else
                                  checkLabel(falseDest[2]),
                                  checkMove(
                                      checkReg(r[2]),
                                      checkBinaryOperation(
                                          ir::BinOp::PLUS,
                                          checkLocalCall(functionLabel,
                                                         checkBinaryOperation(
                                                             ir::BinOp::MINUS,
                                                             checkReg(n),
                                                             checkInt(1))),
                                          checkLocalCall(functionLabel,
                                                         checkBinaryOperation(
                                                             ir::BinOp::MINUS,
                                                             checkReg(n),
                                                             checkInt(2))))),
                                  checkJump(checkLabel(joinDest[2]))),
                              checkLabel(joinDest[2])),
                          checkReg(r[2])))))))(exp);
    }

    SECTION("mutually") {
      auto exp = checkedCompile(R"(
let
  function f() = (g())
  function g() = (h())
  function h() = (f())
in
end
)");

      OptLabel f, h, g;
      checkStatementExpression(checkSequence( // declarations
          checkSequence(                      // functions
              checkSequence(                  // f
                  checkLabel(f), checkMove(checkReg(rv), checkLocalCall(g))),
              checkSequence( // g
                  checkLabel(g), checkMove(checkReg(rv), checkLocalCall(h))),
              checkSequence( // h
                  checkLabel(h), checkMove(checkReg(rv), checkLocalCall(f))))))(
          exp);
    }
  }

  SECTION("nested") {
    auto exp = checkedCompile(R"(
let
  function f(i: int) : int = (
    let
      function g() : int = (
        i + 2
      )
    in
      g()
    end
  )
in
  f(3)
end
)");
    CAPTURE(exp);
    OptReg fp = reg(Registers::RBP);
    OptLabel f, g;
    checkExpressionSequence(   // outer let
        checkSequence(         // declarations
            checkSequence(     // functions
                checkSequence( // f
                    checkLabel(f),
                    checkMove(
                        checkReg(rv),
                        checkExpressionSequence(   // inner let
                            checkSequence(         // declarations
                                checkSequence(     // functions
                                    checkSequence( // g
                                        checkLabel(g),
                                        checkMove(
                                            checkReg(rv),
                                            checkBinaryOperation( // i+2
                                                ir::BinOp::PLUS,
                                                checkMemoryAccess( // i is
                                                                   // fetched
                                                                   // from
                                                                   // f's
                                                                   // frame
                                                    checkBinaryOperation(
                                                        ir::BinOp::PLUS,
                                                        checkMemoryAccess(
                                                            checkBinaryOperation(
                                                                ir::BinOp::PLUS,
                                                                checkReg(fp),
                                                                checkInt(
                                                                    -WORD_SIZE))),
                                                        checkInt(-2 *
                                                                 WORD_SIZE))),
                                                checkInt(2)))))),
                            checkLocalCall(g)))))),
        checkLocalCall( // f(3)
            f, checkInt(3)))(exp);
  }
}

TEST_CASE("standard library") {
  auto checkVoidLibraryCall = [](const std::string name,
                                 const std::string params,
                                 auto &&... checkArgs) {
    auto exp = checkedCompile(name + "(" + params + ")");
    checkLibraryCall(name, std::forward<std::decay_t<decltype(checkArgs)>>(
                               checkArgs)...)(exp);
  };

  auto checkNonVoidLibraryCall =
      [](const std::string returnType, const std::string name,
         const std::string params, auto &&... checkArgs) {
        OptReg v;
        auto exp = checkedCompile("let var v : " + returnType + " := " + name +
                                  "(" + params + ") in end");
        checkStatementExpression(checkSequence( // declarations
            checkMove(checkReg(v),
                      checkLibraryCall(
                          name, std::forward<std::decay_t<decltype(checkArgs)>>(
                                    checkArgs)...))))(exp);
      };

  auto emptyString = "\"\"";

  // function print(s : string)
  SECTION("print") {
    OptLabel stringLabel;
    checkVoidLibraryCall("print", emptyString, checkString(stringLabel));
  }

  // function flush()
  SECTION("flush") { checkVoidLibraryCall("flush", ""); }

  // function getchar() : string
  SECTION("getchar") { checkNonVoidLibraryCall("string", "getchar", ""); }

  // function ord(s: string) : int
  SECTION("ord") {
    OptLabel stringLabel;
    checkNonVoidLibraryCall("int", "ord", emptyString,
                            checkString(stringLabel));
  }

  // function chr(i: int) : string
  SECTION("chr") { checkNonVoidLibraryCall("string", "chr", "0", checkInt(0)); }

  // function size(s: string) : int
  SECTION("size") {
    OptLabel stringLabel;
    checkNonVoidLibraryCall("int", "size", emptyString);
  }

  // function substring(s:string, first:int, n:int) : string
  SECTION("substring") {
    OptLabel stringLabel;
    checkNonVoidLibraryCall("string", "substring", "\"\", 1, 2",
                            checkString(stringLabel), checkInt(1), checkInt(2));
  }

  // function concat(s1: string, s2: string) : string
  SECTION("concat") {
    OptLabel stringLabel[2];
    checkNonVoidLibraryCall("string", "concat", "\"\", \"\"",
                            checkString(stringLabel[0]),
                            checkString(stringLabel[1]));
  }

  // function not(i : integer) : integer
  SECTION("not") { checkNonVoidLibraryCall("int", "not", "3", checkInt(3)); }

  // function exit(i: int)
  SECTION("exit") { checkVoidLibraryCall("exit", "5", checkInt(5)); }
}
