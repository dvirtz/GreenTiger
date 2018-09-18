#include "Program.h"
#include "testsHelper.h"
#include "vectorApply.h"
#include "x64FastCall/Frame.h"
#include "x64FastCall/Registers.h"
#define CATCH_CONFIG_MAIN
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/variant/get.hpp>
#include <catch.hpp>

using namespace tiger;
using namespace tiger::frame::x64FastCall;
using OptReg = boost::optional<temp::Register>;
using OptLabel = boost::optional<temp::Label>;

namespace Catch {
template <> struct StringMaker<ir::Statements> {
  static std::string convert(const ir::Statements &statements) {
    std::ostringstream sstr;
    sstr << "{\n" << statements << '}';
    return sstr.str();
  }
};
} // namespace Catch

static OptReg rv = reg(Registers::RAX);
static OptLabel start{"start"}, end{"end"};
static bool end_checked = false;

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
using helpers::applyFunctionTupleToVector;

enum { WORD_SIZE = Frame::WORD_SIZE };

template <typename T> void setOrCheck(boost::optional<T> &op, T val) {
  if (op) {
    REQUIRE(*op == val);
  } else {
    op = val;
  }
}

auto checkedCompile(const std::string &string) {
  auto res = tiger::compile(string);
  REQUIRE(res);
  return *res;
}

template <typename CheckDest, typename CheckSrc>
auto checkMove(CheckDest &&checkDest, CheckSrc &&checkSrc) {
  return
      [checkDest = std::forward<CheckDest>(checkDest),
       checkSrc = std::forward<CheckSrc>(checkSrc)](const ir::Statement &stm) {
        auto move = get<ir::Move>(&stm);
        REQUIRE(move);
        checkDest(move->dst);
        checkSrc(move->src);
      };
}

auto checkLabel(OptLabel &label) {
  if (&label == &end) {
    end_checked = true;
  }
  return [&label](const auto &exp) {
    auto pLabel = get<temp::Label>(&exp);
    REQUIRE(pLabel);
    setOrCheck(label, *pLabel);
  };
}

auto checkLabel(const std::string &label) {
  return [&label](const auto &exp) {
    auto pLabel = get<temp::Label>(&exp);
    REQUIRE(pLabel);
    REQUIRE(*pLabel == label);
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

template <typename CheckLeft, typename CheckRight>
auto checkBinaryOperation(ir::BinOp op, CheckLeft &&checkLeft,
                          CheckRight &&checkRight) {
  return [op, checkLeft = std::forward<CheckLeft>(checkLeft),
          checkRight =
              std::forward<CheckRight>(checkRight)](const ir::Expression &exp) {
    auto binOperation = get<ir::BinaryOperation>(&exp);
    REQUIRE(binOperation);
    REQUIRE(binOperation->op == op);
    checkLeft(binOperation->left);
    checkRight(binOperation->right);
  };
}

template <typename CheckAddress>
auto checkMemoryAccess(CheckAddress &&checkAddress) {
  return [checkAddress = std::forward<CheckAddress>(checkAddress)](
             const ir::Expression &exp) {
    auto pMemAccess = get<ir::MemoryAccess>(&exp);
    checkAddress(pMemAccess->address);
  };
}

template <typename CheckLabel, typename... CheckArgs>
auto checkCall(CheckLabel &&checkLabel, CheckArgs &&... checkArgs) {
  return [checkLabel = std::forward<CheckLabel>(checkLabel),
          size = sizeof...(checkArgs),
          checkArgs =
              std::forward_as_tuple(checkArgs...)](const ir::Expression &exp) {
    auto call = get<ir::Call>(&exp);
    REQUIRE(call);
    checkLabel(call->fun);
    REQUIRE(call->args.size() == size);
    applyFunctionTupleToVector(call->args, checkArgs);
  };
}

template <size_t level> auto checkStaticLink() {
  return [](const ir::Expression &exp) {
    checkMemoryAccess(checkBinaryOperation(ir::BinOp::PLUS,
                                           checkStaticLink<level - 1>(),
                                           checkInt(-WORD_SIZE)))(exp);
  };
}

template <> auto checkStaticLink<0>() {
  return [](const ir::Expression &exp) {
    OptReg fp = reg(Registers::RBP);
    checkReg(fp)(exp);
  };
}

template <typename... CheckArgs>
auto checkLocalCall(OptLabel &label, CheckArgs &&... checkArgs) {
  return checkCall(checkLabel(label), checkStaticLink<1>(),
                   std::forward<CheckArgs>(checkArgs)...);
}

template <typename... CheckArgs>
auto checkExternalCall(const std::string &name, CheckArgs &&... checkArgs) {
  return checkCall(checkLabel(name), std::forward<CheckArgs>(checkArgs)...);
}

template <typename CheckExp>
auto checkExpressionStatement(CheckExp &&checkExp) {
  return
      [checkExp = std::forward<CheckExp>(checkExp)](const ir::Statement &stm) {
        auto expStatement = get<ir::ExpressionStatement>(&stm);
        REQUIRE(expStatement);
        checkExp(expStatement->exp);
      };
}

// a nop is implemented as 0
auto checkNop() { return checkExpressionStatement(checkInt(0)); }

template <typename CheckLeft, typename CheckRight>
auto checkConditionalJump(ir::RelOp relOp, CheckLeft &&checkLeft,
                          CheckRight &&checkRight, OptLabel *trueDest = nullptr,
                          OptLabel *falseDest = nullptr) {
  return [relOp, checkLeft = std::forward<CheckLeft>(checkLeft),
          checkRight = std::forward<CheckRight>(checkRight), trueDest,
          falseDest](const ir::Statement &stm) {
    auto pCondJump = get<ir::ConditionalJump>(&stm);
    REQUIRE(pCondJump);
    REQUIRE(pCondJump->op == relOp);
    checkLeft(pCondJump->left);
    checkRight(pCondJump->right);
    auto checkDest = [](OptLabel *expected,
                        const std::shared_ptr<temp::Label> &actual) {
      if (expected) {
        REQUIRE(actual);
        setOrCheck(*expected, *actual);
      } else {
        REQUIRE_FALSE(actual);
      }
    };

    checkDest(trueDest, pCondJump->trueDest);
    checkDest(falseDest, pCondJump->falseDest);
  };
}

auto checkString(OptLabel &stringLabel) {
  // a string is translated to the label of the string address
  return checkLabel(stringLabel);
}

template <typename CheckExp> auto checkJump(CheckExp &&checkExp) {
  return
      [checkExp = std::forward<CheckExp>(checkExp)](const ir::Statement &stm) {
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
  return checkBinaryOperation(ir::BinOp::PLUS, checkReg(r),
                              checkBinaryOperation(ir::BinOp::MUL,
                                                   checkInt(index),
                                                   checkInt(WORD_SIZE)));
}

auto checkMemberAccess(OptReg &r, int index) {
  return checkMemoryAccess(checkMemberAddress(r, index));
}

template <typename Tuple> auto tupleSize(const Tuple &/* t */) {
  return std::tuple_size<Tuple>::value;
}

template <typename... CheckStatements>
auto checkProgram(CheckStatements &&... checkStatements) {
  return [checkStatements = std::make_tuple(
              checkLabel(start),
              std::forward<CheckStatements>(checkStatements)...)](
             const ir::Statements &statements) {
    CAPTURE(statements);

    if (end_checked) {
      REQUIRE(statements.size() == tupleSize(checkStatements));
      applyFunctionTupleToVector(statements, checkStatements);
    } else {
      REQUIRE(statements.size() == tupleSize(checkStatements) + 1);
      applyFunctionTupleToVector(
          statements,
        checkStatements);
      checkJump(checkLabel(end))(statements.back());
    }

    end_checked = false;
  };
}

TEST_CASE("lvalue") {
  SECTION("identifier") {
    auto program = checkedCompile(R"(
let
  var i := 0
in
  i
end
)");
    OptReg i;
    checkProgram(checkMove( // allocates a register and set it to 0
                     checkReg(i), checkInt(0)),
                 checkMove( // return previously allocated reg
                     checkReg(rv), checkReg(i)))(program);
  }
  SECTION("field") {
    auto program = checkedCompile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.i
end
)");
    OptReg r;
    checkProgram(checkMove( // allocates a register and set it to 0
                     checkReg(r), checkInt(0)),
                 checkMove( // returns the offset of i in rec relative to
                            // previously allocated reg
                     checkReg(rv), checkMemberAccess(r, 0)))(program);
  }
  SECTION("array element") {
    auto program = checkedCompile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[1]
end
)");
    OptReg reg1, reg2;
    checkProgram(
        checkMove( // set reg2 to result of malloc(array size),
            checkReg(reg2),
            checkExternalCall("malloc", checkBinaryOperation(
                                            ir::BinOp::MUL, checkInt(WORD_SIZE),
                                            checkInt(2)))),
        checkExpressionStatement(checkExternalCall( // call initArray(array
                                                    // size, array init val)
            "initArray", checkInt(2), checkInt(3))),
        checkMove(checkReg(reg1), // move reg2 to reg1,
                  checkReg(reg2)),
        checkMove( // returns the offset of element 1 in arr relative
                   // to reg1
            checkReg(rv), checkMemberAccess(reg1, 1)))(program);
  }
}

TEST_CASE("nil") {
  auto program = checkedCompile("nil");
  checkProgram(checkMove(checkReg(rv), checkInt(0)))(program);
}

TEST_CASE("sequence") {
  SECTION("empty") {
    auto program = checkedCompile("()");
    checkProgram(checkMove(checkReg(rv), checkInt(0)))(program);
  }

  SECTION("single") {
    auto program = checkedCompile("(42)");
    checkProgram(checkMove(checkReg(rv), checkInt(42)))(program);
  }

  SECTION("multiple") {
    // no-op expressions are removed
    auto program = checkedCompile(R"((1;"two";flush()))");
    OptLabel flush = temp::Label{"flush"};
    checkProgram(checkMove(checkReg(rv), checkLocalCall(flush)))(program);
  }
}

TEST_CASE("integer") {
  auto program = checkedCompile("42");
  checkProgram(checkMove(checkReg(rv), checkInt(42)))(program);
}

TEST_CASE("string") {
  auto str = R"("\tHello \"World\"!\n")";
  auto program = checkedCompile(str);
  OptLabel stringLabel;
  checkProgram(checkMove(checkReg(rv), checkString(stringLabel)))(program);
}

TEST_CASE("function call") {
  OptLabel functionLabel, functionEnd;

  SECTION("with no arguments") {
    auto program = checkedCompile(R"(
let
  function f() = ()
in
  f()
end
)");
    checkProgram(checkMove(checkReg(rv), checkLocalCall(functionLabel) // call
                           ),
                 checkJump(checkLabel(end)),
                 checkLabel(functionLabel), // function label
                 checkMove(                 // body
                     checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(functionEnd)))(program);
  }

  SECTION("with arguments") {
    OptReg rcx = reg(Registers::RCX);
    auto program = checkedCompile(R"(
let
  function f(a: int) = ()
in
  f(2)
end
)");
    checkProgram(checkMove(checkReg(rv), checkLocalCall( // call
                                             functionLabel, checkInt(2))),
                 checkJump(checkLabel(end)),
                 checkLabel(functionLabel), // function label,
                 checkMove(                 // body
                     checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(functionEnd)))(program);
  }
}

TEST_CASE("record") {
  SECTION("empty") {
    auto program = checkedCompile(R"(
let 
  type t = {}
in
  t{}
end
)");
    OptReg r;
    checkProgram(checkMove( // move result of malloc(record_size) to
                     checkReg(r), checkExternalCall("malloc", checkInt(0))),
                 checkMove(checkReg(rv), checkReg(r)))(program);
  }

  SECTION("not empty") {
    auto program = checkedCompile(R"(
let 
  type t = {a: int, b: string}
in
  t{a = 2, b = "hello"}
end
)");

    OptReg r;
    OptLabel stringLabel;
    checkProgram(
        checkMove( // move result of malloc(record_size) to r
            checkReg(r), checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
        checkMove( // init first member with 2
            checkMemberAccess(r, 0), checkInt(2)),
        checkMove( // init second member with "hello"
            checkMemberAccess(r, 1), checkString(stringLabel)),
        checkMove(checkReg(rv), checkReg(r)))(program);
  }
}

TEST_CASE("array") {
  auto program = checkedCompile(R"(
let
  type t = array of int
in
  t[2] of 3
end
)");
  OptReg r;
  checkProgram(
      checkMove( // move result of malloc(array_size) to r
          checkReg(r),
          checkExternalCall("malloc", checkBinaryOperation(ir::BinOp::MUL,
                                                           checkInt(WORD_SIZE),
                                                           checkInt(2)))),
      checkExpressionStatement( // call initArray(array_size, init_val)
          checkExternalCall("initArray", checkInt(2), checkInt(3))),
      checkMove(checkReg(rv), checkReg(r)))(program);
}

TEST_CASE("arithmetic") {
  static const std::pair<std::string, ir::BinOp> operations[] = {
      {"+", ir::BinOp::PLUS},
      {"-", ir::BinOp::MINUS},
      {"*", ir::BinOp::MUL},
      {"/", ir::BinOp::DIV}};

  for (const auto &operation : operations) {
    SECTION(Catch::StringMaker<ir::BinOp>::convert(operation.second)) {
      auto program = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
    i
  end
  )") % operation.first));
      OptReg r;
      checkProgram(checkMove(checkReg(r), // move result of 2 op 3 to a r
                             checkBinaryOperation(operation.second, checkInt(2),
                                                  checkInt(3))),
                   checkMove(checkReg(rv), checkReg(r) // return i
                             ))(program);
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
        auto program = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
    i
  end
  )") % relation.first));
        OptReg i, r;
        OptLabel trueDest, falseDest;
        checkProgram(checkMove(checkReg(r),
                               checkInt(1)), // move 1 to r
                     checkConditionalJump(relation.second, checkInt(2),
                                          checkInt(3), &trueDest,
                                          &falseDest), // check condition,
                                                       // jumping to trueDest
                                                       // or falseDest
                                                       // accordingly
                     checkLabel(falseDest),
                     checkMove(checkReg(r),
                               checkInt(0)), // if condition is false,
                                             // move 0 to r
                     checkLabel(trueDest),
                     checkMove(checkReg(i),
                               checkReg(r) // move r to i
                               ),
                     checkMove(checkReg(rv), checkReg(i) // return i
                               ))(program);
      }
    }
  }

  SECTION("string") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto program = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := "2" %1% "3"
  in
    i
  end
  )") % relation.first));
        OptReg r1, r2, r3;
        OptLabel trueDest, falseDest;
        OptLabel leftString, rightString;
        checkProgram(checkMove(checkReg(r2),
                               checkInt(1)), // move 1 to r2
                     checkMove(checkReg(r3),
                               checkExternalCall("stringCompare",
                                                 checkString(leftString),
                                                 checkString(rightString))),
                     checkConditionalJump( // stringCompare(s1, s2)
                                           // returns -1, 0 or 1 when s1
                                           // is less than, equal or
                                           // greater than s2,
                                           // respectively so s1 op s2 is
                                           // the same as
                                           // stringCompare(s1, s2) op 0
                         relation.second, checkReg(r3), checkInt(0), &trueDest,
                         &falseDest), // check condition,
                                      // jumping to trueDest
                                      // or falseDest
                                      // accordingly
                     checkLabel(falseDest),
                     checkMove(checkReg(r2),
                               checkInt(0)), // if condition is false,
                                             // move 0 to r2
                     checkLabel(trueDest),
                     checkMove(checkReg(r1),
                               checkReg(r2) // move r2 to r1
                               ),
                     checkMove(checkReg(rv),
                               checkReg(r1) // return i
                               ))(program);
      }
    }
  }

  static const std::pair<std::string, ir::RelOp> eqNeq[] = {
      {"=", ir::RelOp::EQ}, {"<>", ir::RelOp::NE}};

  SECTION("record") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto program = checkedCompile(boost::str(boost::format(R"(
let
  type t = {}
  var i := t{}
in
  i %1% i
end
)") % relation.first));
        OptReg i, r1, r2;
        OptLabel trueDest, falseDest;
        checkProgram(
            checkMove( // move result of
                       // malloc(record_size) to r
                checkReg(r1), checkExternalCall("malloc", checkInt(0))),
            checkMove( // move record to i
                checkReg(i), checkReg(r1)),
            checkMove(checkReg(r2),
                      checkInt(1)), // move 1 to r2
            checkConditionalJump(relation.second, checkReg(i), checkReg(i),
                                 &trueDest,
                                 &falseDest), // check condition,
                                              // jumping to trueDest
                                              // or falseDest
                                              // accordingly
            checkLabel(falseDest),
            checkMove(checkReg(r2),
                      checkInt(0)), // if condition is false, move 0 to r2
            checkLabel(trueDest),
            checkMove(checkReg(rv),
                      checkReg(r2) // move r2 to r1
                      ))(program);
      }
    }
  }

  SECTION("array") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto program = checkedCompile(boost::str(boost::format(R"(
let
  type t = array of int
  var i := t[2] of 3
in
  i %1% i
end
)") % relation.first));
        OptReg i, r1, r2;
        OptLabel trueDest, falseDest;
        checkProgram(
            checkMove( // move result of malloc(array_size) to
                       // r1
                checkReg(r1),
                checkExternalCall("malloc",
                                  checkBinaryOperation(ir::BinOp::MUL,
                                                       checkInt(WORD_SIZE),
                                                       checkInt(2)))),
            checkExpressionStatement(
                checkExternalCall( // call
                                   // initArray(array_size, init_val)
                    "initArray", checkInt(2), checkInt(3))),
            checkMove( // move array to i
                checkReg(i), checkReg(r1)),
            checkMove(checkReg(r2),
                      checkInt(1)), // move 1 to r2
            checkConditionalJump(relation.second, checkReg(i), checkReg(i),
                                 &trueDest,
                                 &falseDest), // check condition,
                                              // jumping to trueDest
                                              // or falseDest
                                              // accordingly
            checkLabel(falseDest),
            checkMove(checkReg(r2),
                      checkInt(0)), // if condition is false, move 0 to r2
            checkLabel(trueDest),
            checkMove(checkReg(rv),
                      checkReg(r2) // move r2 to r1
                      ))(program);
      }
    }
  }
}

TEST_CASE("boolean") {
  SECTION("and") {
    auto program = checkedCompile("2 & 3");
    // a & b is translated to if a then b else 0
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkProgram(
        // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf), its
        // "false branch."
        checkConditionalJump( // test
            ir::RelOp::NE, checkInt(2), checkInt(0), &trueDest, &falseDest),
        checkLabel(falseDest),               // else
        checkMove(checkReg(r), checkInt(0)), // 0
        checkLabel(joinDest),                // branches join here
        checkMove(checkReg(rv), checkReg(r)), checkJump(checkLabel(end)),
        checkLabel(trueDest),                // then
        checkMove(checkReg(r), checkInt(3)), // 3
        checkJump(checkLabel(joinDest)))(program);
  }

  SECTION("or") {
    auto program = checkedCompile("2 | 3");
    // a | b is translated to if a then 1 else b
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkProgram(
        // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf), its
        // "false branch."
        checkConditionalJump( // test
            ir::RelOp::NE, checkInt(2), checkInt(0), &trueDest, &falseDest),
        checkLabel(falseDest),               // else
        checkMove(checkReg(r), checkInt(3)), // 3
        checkLabel(joinDest),                // branches join here
        checkMove(checkReg(rv), checkReg(r)), checkJump(checkLabel(end)),
        checkLabel(trueDest),                // then
        checkMove(checkReg(r), checkInt(1)), // 1
        checkJump(checkLabel(joinDest)))(program);
  }
}

TEST_CASE("assignment") {
  auto program = checkedCompile(R"(
let
  var a := 3
  var b := 0
in
  b := a
end
)");
  OptReg a, b;
  checkProgram(checkMove( // move 3 to a
                   checkReg(a), checkInt(3)),
               checkMove( // move 0 to b
                   checkReg(b), checkInt(0)),
               checkMove( // move b to a
                   checkReg(b), checkReg(a)),
               checkMove( // return 0
                   checkReg(rv), checkInt(0)))(program);
}

TEST_CASE("if then") {
  OptReg a;
  OptLabel trueDest, falseDest, joinDest;
  
  SECTION("no else") {
    auto program = checkedCompile(R"(
let
  var a := 0
in
  if 1 then a := 3
end
)");
    checkProgram(
        // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf), its
        // "false branch."
        checkMove( // move 0 to a
            checkReg(a), checkInt(0)),
        checkConditionalJump( // test
            ir::RelOp::NE, checkInt(1), checkInt(0), &trueDest, &falseDest),
        checkLabel(falseDest),
        checkLabel(joinDest), // branches join here
        checkMove(            // return 0
            checkReg(rv), checkInt(0)),
          checkJump(checkLabel(end)),
          checkLabel(trueDest),
        checkMove( // move 3 to a
            checkReg(a), checkInt(3)),
            checkJump(checkLabel(joinDest)))(program);
  }

  SECTION("with else") {
    SECTION("both expressions are of type void") {
      auto program = checkedCompile(R"(
let
  var a := 0
in
  if 0 then a := 3 else a := 4
end
)");
      checkProgram(
          // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf),
          // its "false branch."
          checkMove( // move 0 to a
              checkReg(a), checkInt(0)),
          checkConditionalJump( // test
              ir::RelOp::NE, checkInt(0), checkInt(0), &trueDest, &falseDest),
          checkLabel(falseDest), // else
          checkMove(checkReg(a),
                    checkInt(4)), // move 4 to a
          checkLabel(joinDest),   // branches join here
          checkMove(              // return 0
              checkReg(rv), checkInt(0)),
          checkJump(checkLabel(end)),
          checkLabel(trueDest), // then
          checkMove(checkReg(a),
                    checkInt(3)), // move 3 to a
          checkJump(checkLabel(joinDest)))(program);
    }

    SECTION("both expressions are of the same type") {
      auto program = checkedCompile(R"(
let
  var a := if 1 then 2 else 3
in
  a
end
)");
      OptReg r;
      checkProgram(
          // every CJUMP(cond, lt , lf) is immediately followed by LABEL(lf),
          // its "false branch."
          checkConditionalJump( // test
              ir::RelOp::NE, checkInt(1), checkInt(0), &trueDest, &falseDest),
          checkLabel(falseDest), // else
          checkMove(checkReg(r),
                    checkInt(3)), // move 3 to r
          checkLabel(joinDest),
          checkMove( // move r to a
              checkReg(a), checkReg(r)),
          checkMove( // return a
              checkReg(rv), checkReg(a)),
          checkJump(checkLabel(end)),
          checkLabel(trueDest), // then
          checkMove(checkReg(r),
                    checkInt(2)), // move 2 to r
          checkJump(checkLabel(joinDest)))(program);
    }
  }
}

TEST_CASE("while") {
  auto program = checkedCompile(R"(
let
  var b := 2
  var a := 0
in
  while b > 0 do (a:=a+1)
end
)");
  OptReg a, b, r;
  OptLabel loopStart, loopDone, trueDest, falseDest, loopNotDone;
  checkProgram(
      checkMove( // move 2 to b
          checkReg(b), checkInt(2)),
      checkMove( // move 0 to a
          checkReg(a), checkInt(0)),
      checkLabel(loopStart),
      checkMove(checkReg(r),
                checkInt(1)), // move 1 to r
      checkConditionalJump(ir::RelOp::GT, checkReg(b), checkInt(0), &trueDest,
                           &falseDest), // check condition,
                                        // jumping to trueDest
                                        // or falseDest
                                        // accordingly
      checkLabel(falseDest),
      checkMove(checkReg(r),
                checkInt(0)), // if condition is false, move 0 to r
      checkLabel(trueDest),
      checkConditionalJump( // test
          ir::RelOp::EQ, checkReg(r), checkInt(0), &loopDone,
          &loopNotDone // if test equals 0 jumps to loopDone
          ), 
      checkLabel(loopNotDone),
      checkMove( // a := a + 1
          checkReg(a),
          checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
      checkJump( // jump back to loop Start
          checkLabel(loopStart)),
      checkLabel(loopDone),
      checkMove( // return 0
          checkReg(rv), checkInt(0)))(program);
}

TEST_CASE("for") {
  auto program = checkedCompile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)");
  OptReg a, limit, rcx = reg(Registers::RCX);
  OptLabel functionLabel, loopStart, loopDone, functionEnd;
  checkProgram(
      checkMove( // move 0 to a
          checkReg(a), checkInt(0)),
      checkMove( // move 4 to limit
          checkReg(limit), checkInt(4)),
      checkConditionalJump( // skip the loop if a > limit
          ir::RelOp::GT, checkReg(a), checkReg(limit), &loopDone, &loopStart), checkLabel(loopStart),
      checkExpressionStatement(checkLocalCall( // f(a)
          functionLabel, checkReg(a))),
      checkMove( // a := a + 1
          checkReg(a),
          checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
      checkConditionalJump(ir::RelOp::LT, checkReg(a), checkReg(limit),
                           &loopStart, &loopDone),
      checkLabel(loopDone), checkMove(checkReg(rv), checkInt(0)),
      checkJump(checkLabel(end)),
      checkLabel(functionLabel), // function label
      checkMove(                 // body
          checkReg(rv), checkInt(0)),
      checkJump(checkLabel(functionEnd)))(program);
}

TEST_CASE("break") {
  SECTION("for") {
    auto program = checkedCompile("for a := 0 to 2 do break");
    OptReg a, limit;
    OptLabel functionLabel, loopStart, loopDone, afterBreak, postLoop;
    checkProgram(
        checkMove( // move 0 to a
            checkReg(a), checkInt(0)),
        checkMove( // move 4 to limit
            checkReg(limit), checkInt(2)),
        checkConditionalJump( // skip the loop if a > limit
            ir::RelOp::GT, checkReg(a), checkReg(limit), &loopDone, &loopStart), checkLabel(loopStart),
        checkLabel(loopDone), checkMove(checkReg(rv), checkInt(0)),
        checkJump(checkLabel(end)),
        checkLabel(afterBreak),
        checkMove( // a := a + 1
            checkReg(a),
            checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
        checkConditionalJump(ir::RelOp::LT, checkReg(a), checkReg(limit),
                             &loopStart, &postLoop),
        checkLabel(postLoop), checkJump(checkLabel(loopDone)))(program);
  }

  SECTION("while") {
    auto program = checkedCompile("while 1 do break");
    OptReg a, b, r;
    OptLabel loopStart, loopDone, loopNotDone, afterBreak;
    checkProgram(checkLabel(loopStart),
                 checkConditionalJump( // test
                     ir::RelOp::EQ, checkInt(1), checkInt(0), &loopDone, &loopNotDone // if test equals 0 jumps to loopDone
                     ),
                     checkLabel(loopNotDone), 
                 checkLabel(loopDone), checkMove(checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(end)), 
                 checkLabel(afterBreak),
                 checkJump( // jump back to loop Start
                     checkLabel(loopStart)))(program);
  }
}

TEST_CASE("type declarations") {
  SECTION("name") {
    auto program = checkedCompile(R"(
let
  type a = int
  var b : a := 0
in
end
)");
    OptReg b;
    checkProgram(checkMove( // move 0 to b
                     checkReg(b), checkInt(0)),
                 checkMove(checkReg(rv), checkInt(0)))(program);
  }

  SECTION("record") {
    auto program = checkedCompile(R"(
let
  type rec = {i : int}
  var r := rec{i=2}
in
end
)");
    OptReg r1, r2;
    checkProgram(
        checkMove( // move result of malloc(record_size) to r2
            checkReg(r2), checkExternalCall("malloc", checkInt(WORD_SIZE))),
        checkMove( // init first member with 2
            checkMemberAccess(r2, 0), checkInt(2)),
        checkMove( // move r2 to r1
            checkReg(r1), checkReg(r2)),
        checkMove(checkReg(rv), checkInt(0)))(program);
  }

  SECTION("array") {
    auto program = checkedCompile(R"(
let
  type arr = array of string
  var a := arr[3] of "a"
in
end
)");
    OptReg r1, r2;
    OptLabel stringLabel;
    checkProgram(
        checkMove( // set reg2 to result of malloc(array size),
            checkReg(r2),
            checkExternalCall("malloc", checkBinaryOperation(
                                            ir::BinOp::MUL, checkInt(WORD_SIZE),
                                            checkInt(3)))),
        checkExpressionStatement(checkExternalCall( // call initArray(array
                                                    // size, array init val)
            "initArray", checkInt(3), checkString(stringLabel))),
        checkMove(checkReg(r1), // move reg2 to reg1
                  checkReg(r2)),
        checkMove(checkReg(rv), checkInt(0)))(program);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto program = checkedCompile(R"(
let
  type intlist = {hd: int, tl: intlist}
in
  intlist{hd = 3, tl = intlist{hd = 4, tl = nil}}
end
)");
      OptReg r1, r2, r3;
      checkProgram(checkMove( // move result of malloc(record_size) to r1
                       checkReg(r1),
                       checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                   checkMove( // init first member
                       checkMemberAccess(r1, 0), checkInt(3)),
                   checkMove( // move address of second member to r2
                       checkReg(r2), checkMemberAddress(r1, 1)),
                   checkMove( // move result of malloc(record_size)
                              // to r3
                       checkReg(r3),
                       checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                   checkMove( // init first member
                       checkMemberAccess(r3, 0), checkInt(4)),
                   checkMove( // init second member
                       checkMemberAccess(r3, 1), checkInt(0)),
                   checkMove( // init second member
                       checkMemoryAccess(checkReg(r2)), checkReg(r3)),
                   checkMove(checkReg(rv), checkReg(r1)))(program);
    }

    SECTION("mutually") {
      auto program = checkedCompile(R"(
let
  type tree = {key: int, children: treelist}
  type treelist = {hd: tree, tl: treelist}
in
  tree{key = 1, children = treelist{hd = tree{key = 2, children = nil}, tl = nil}}
end
)");
      OptReg r1, r2, r3, r4, r5;
      checkProgram(checkMove( // move result of malloc(record_size) to r1
                       checkReg(r1),
                       checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                   checkMove( // init first member
                       checkMemberAccess(r1, 0), checkInt(1)),
                   checkMove( // move address of second member to r2
                       checkReg(r4), checkMemberAddress(r1, 1)),
                   checkMove( // move result of malloc(record_size)
                              // to r2
                       checkReg(r2),
                       checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                   checkMove( // move address of first member to r5
                       checkReg(r5), checkMemberAddress(r2, 0)),
                   checkMove( // move result of
                              // malloc(record_size) to
                              // r3
                       checkReg(r3),
                       checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                   checkMove( // init first member
                       checkMemberAccess(r3, 0), checkInt(2)),
                   checkMove( // init second member
                       checkMemberAccess(r3, 1), checkInt(0)),
                   checkMove( // init first member
                       checkMemoryAccess(checkReg(r5)), checkReg(r3)),
                   checkMove( // init second member
                       checkMemberAccess(r2, 1), checkInt(0)),
                   checkMove( // init second member
                       checkMemoryAccess(checkReg(r4)), checkReg(r2)),
                   checkMove(checkReg(rv), checkReg(r1)))(program);
    }
  }
}

TEST_CASE("function declarations") {

  SECTION("simple") {
    auto program = checkedCompile(R"(
let
  function f() = ()
in
end
)");
    OptLabel functionLabel, functionEnd;
    checkProgram(checkMove(checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(end)),
                 checkLabel(functionLabel), // function label
                 checkMove(                 // body
                     checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(functionEnd)))(program);
  }

  SECTION("with parameters") {
    auto program = checkedCompile(R"(
let
  function f(a:int, b:string) = ()
in
end
)");
    OptLabel functionLabel, functionEnd;
    checkProgram(checkMove(checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(end)),
                 checkLabel(functionLabel), // function label
                 checkMove(                 // body
                     checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(functionEnd)))(program);
  }

  SECTION("with return value") {
    auto program = checkedCompile(R"(
let
  function f(a:int, b:string) : string = (a:=1;b)
in
end
)");
    OptReg a, b;
    OptLabel functionLabel, functionEnd;
    checkProgram(checkMove(checkReg(rv), checkInt(0)),
                 checkJump(checkLabel(end)),
                 checkLabel(functionLabel), // function label
                 checkMove(                 // body
                     checkReg(a), checkInt(1)),
                 checkMove(checkReg(rv), checkReg(b)),
                 checkJump(checkLabel(functionEnd)))(program);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto program = checkedCompile(R"(
let
  function fib(n:int) : int = (
    if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
  )
in
end
)");
      OptReg rcx = reg(Registers::RCX), n, r[6];
      OptLabel functionLabel, functionEnd, trueDest[3], falseDest[3],
          joinDest[3];
      checkProgram(
          checkMove(checkReg(rv), checkInt(0)), checkJump(checkLabel(end)),
          checkLabel(functionLabel), // function label
          checkConditionalJump(      // n = 0
              ir::RelOp::EQ, checkReg(n), checkInt(0), &trueDest[0],
              &falseDest[0]),
          checkLabel( // else
              falseDest[0]),
          checkMove(checkReg(r[1]), checkInt(1)),
          checkConditionalJump( // n
                                // =
                                // 1
              ir::RelOp::EQ, checkReg(n), checkInt(1), &trueDest[1],
              &falseDest[1]),
          checkLabel(falseDest[1]), checkMove(checkReg(r[1]), checkInt(0)),
          checkLabel(trueDest[1]), checkMove(checkReg(r[0]), checkReg(r[1])),
          checkLabel(joinDest[0]),
          checkConditionalJump( // test
              ir::RelOp::NE, checkReg(r[0]), checkInt(0), &trueDest[2],
              &falseDest[2]),
          checkLabel( // else
              falseDest[2]),
          checkMove( // fib(n-1)
              checkReg(r[3]),
              checkCall(checkLabel(functionLabel), checkStaticLink<2>(),
                        checkBinaryOperation(ir::BinOp::MINUS, checkReg(n),
                                             checkInt(1)))),
          checkMove(checkReg(r[5]), checkReg(r[3])),
          checkMove( // fib(n-2)
              checkReg(r[4]),
              checkCall(checkLabel(functionLabel), checkStaticLink<2>(),
                        checkBinaryOperation(ir::BinOp::MINUS, checkReg(n),
                                             checkInt(2)))),
          checkMove( // add two previous calls
              checkReg(r[2]),
              checkBinaryOperation(ir::BinOp::PLUS, checkReg(r[5]),
                                   checkReg(r[4]))),
          checkLabel(joinDest[2]),
          checkMove( // return result
              checkReg(rv), checkReg(r[2])),
          checkJump(checkLabel(functionEnd)),
          checkLabel( // then
              trueDest[0]),
          checkMove(checkReg(r[0]), checkInt(1)),
          checkJump(checkLabel(joinDest[0])),
          checkLabel( // then
              trueDest[2]),
          checkMove(checkReg(r[2]), checkReg(n)),
          checkJump(checkLabel(joinDest[2])))(program);
    }

    SECTION("mutually") {
      auto program = checkedCompile(R"(
let
  function f(x:int) = (g(1))
  function g(x:int) = (h(2))
  function h(x:int) = (f(3))
in
end
)");
      OptLabel fStart, fEnd, hStart, hEnd, gStart, gEnd;
      checkProgram(
          checkMove(checkReg(rv), checkInt(0)), checkJump(checkLabel(end)),
          checkLabel(fStart),
          checkMove(checkReg(rv), checkCall(checkLabel(gStart),
                                            checkStaticLink<2>(), checkInt(1))),
          checkJump(checkLabel(fEnd)), checkLabel(gStart),
          checkMove(checkReg(rv), checkCall(checkLabel(hStart),
                                            checkStaticLink<2>(), checkInt(2))),
          checkJump(checkLabel(gEnd)), checkLabel(hStart),
          checkMove(checkReg(rv), checkCall(checkLabel(fStart),
                                            checkStaticLink<2>(), checkInt(3))),
          checkJump(checkLabel(hEnd)))(program);
    }
  }

  SECTION("nested") {
    auto program = checkedCompile(R"(
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
    OptReg fp = reg(Registers::RBP), rcx = reg(Registers::RCX);
    OptLabel f, fEnd, g, gEnd;
    checkProgram(checkMove(checkReg(rv),
                           checkLocalCall( // f(3)
                               f, checkInt(3))),
                 checkJump(checkLabel(end)), checkLabel(g),
                 checkMove(checkReg(rv),
                           checkBinaryOperation( // i+2
                               ir::BinOp::PLUS,
                               checkMemoryAccess( // i is
                                                  // fetched
                                                  // from
                                                  // f's
                                                  // frame
                                   checkBinaryOperation(
                                       ir::BinOp::PLUS,
                                       checkMemoryAccess(checkBinaryOperation(
                                           ir::BinOp::PLUS, checkReg(fp),
                                           checkInt(-WORD_SIZE))),
                                       checkInt(-2 * WORD_SIZE))),
                               checkInt(2))),
                 checkJump(checkLabel(gEnd)), checkLabel(f),
                 checkMove(checkReg(rv), checkLocalCall(g)),
                 checkJump(checkLabel(fEnd)))(program);
  }
}

TEST_CASE("standard library") {
  auto checkVoidLibraryCall = [](const std::string name,
                                 const std::string params,
                                 auto &&... checkArgs) {
    OptLabel label = temp::Label{name};
    auto program = checkedCompile(name + "(" + params + ")");
    checkProgram(checkMove(
        checkReg(rv),
        checkLocalCall(label, std::forward<std::decay_t<decltype(checkArgs)>>(
                                  checkArgs)...)))(program);
  };

  auto checkNonVoidLibraryCall = [](const std::string returnType,
                                    const std::string name,
                                    const std::string params,
                                    auto &&... checkArgs) {
    OptReg v;
    OptLabel label = temp::Label{name};
    auto program = checkedCompile("let var v : " + returnType + " := " + name +
                                  "(" + params + ") in v end");
    checkProgram(
        checkMove(checkReg(v),
                  checkLocalCall(
                      label, std::forward<std::decay_t<decltype(checkArgs)>>(
                                 checkArgs)...)),
        checkMove(checkReg(rv), checkReg(v)))(program);
  };

  using namespace std::string_literals;
  auto emptyString = "\"\""s;

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
    checkNonVoidLibraryCall("int", "size", emptyString,
                            checkString(stringLabel));
  }

  // function substring(s:string, first:int, n:int) : string
  SECTION("substring") {
    OptLabel stringLabel;
    checkNonVoidLibraryCall("string", "substring", emptyString + ", 1, 2",
                            checkString(stringLabel), checkInt(1), checkInt(2));
  }

  // function concat(s1: string, s2: string) : string
  SECTION("concat") {
    OptLabel stringLabel[2];
    checkNonVoidLibraryCall(
        "string", "concat", emptyString + ", " + emptyString,
        checkString(stringLabel[0]), checkString(stringLabel[1]));
  }

  // function not(i : integer) : integer
  SECTION("not") { checkNonVoidLibraryCall("int", "not", "3", checkInt(3)); }

  // function exit(i: int)
  SECTION("exit") { checkVoidLibraryCall("exit", "5", checkInt(5)); }
}
