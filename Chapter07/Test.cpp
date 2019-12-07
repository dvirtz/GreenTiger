#include "Program.h"
#include "testsHelper.h"
#include "vectorApply.h"
#include "x64FastCall/Frame.h"
#include "x64FastCall/Registers.h"
#define CATCH_CONFIG_MAIN
#include <boost/format.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/variant/get.hpp>
#include <catch2/catch.hpp>

using namespace tiger;
using namespace tiger::frame::x64FastCall;
using OptReg   = boost::optional<temp::Register>;
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
using helpers::applyFunctionTupleToVector;

enum { WORD_SIZE = Frame::WORD_SIZE };

template <typename T> void setOrCheck(boost::optional<T> &op, T val) {
  if (op) {
    REQUIRE(*op == val);
  } else {
    op = val;
  }
}

std::pair<ir::Expression, FragmentList>
  checkedCompile(const std::string &string) {
  auto res = tiger::compile(string);
  REQUIRE(res);
  return *res;
}

template <typename CheckStatement, typename CheckExpression>
auto checkExpressionSequence(CheckStatement &&checkStatement,
                             CheckExpression &&checkExpression) {
  return [checkStatement  = std::forward<CheckStatement>(checkStatement),
          checkExpression = std::forward<CheckExpression>(checkExpression)](
           const ir::Expression &exp) {
    auto seq = get<ir::ExpressionSequence>(&exp);
    REQUIRE(seq);
    checkStatement(seq->stm);
    checkExpression(seq->exp);
  };
}

template <typename... CheckStatements>
auto checkSequence(CheckStatements &&... checkStatements) {
  return [size            = sizeof...(checkStatements),
          checkStatements = std::forward_as_tuple(std::forward<CheckStatements>(
            checkStatements)...)](const ir::Statement &stm) {
    auto seq = get<ir::Sequence>(&stm);
    REQUIRE(seq);
    REQUIRE(seq->statements.size() == size);
    applyFunctionTupleToVector(seq->statements, checkStatements);
  };
}

template <typename CheckDest, typename CheckSrc>
auto checkMove(CheckDest &&checkDest, CheckSrc &&checkSrc) {
  return
    [checkDest = std::forward<CheckDest>(checkDest),
     checkSrc  = std::forward<CheckSrc>(checkSrc)](const ir::Statement &stm) {
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
          size       = sizeof...(checkArgs),
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
                          CheckRight &&checkRight, OptLabel &trueDest,
                          OptLabel *falseDest = nullptr) {
  return [relOp, checkLeft = std::forward<CheckLeft>(checkLeft),
          checkRight = std::forward<CheckRight>(checkRight), &trueDest,
          falseDest](const ir::Statement &stm) {
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

auto checkMemberAccess(OptReg &r, int index) {
  return checkMemoryAccess(
    checkBinaryOperation(ir::BinOp::PLUS, checkReg(r),
                         checkBinaryOperation(ir::BinOp::MUL, checkInt(index),
                                              checkInt(WORD_SIZE))));
}

template <typename... CheckFragments>
auto checkFragments(CheckFragments &&... checkFragments) {
  return [size           = sizeof...(checkFragments),
          checkFragments = std::forward_as_tuple(std::forward<CheckFragments>(
            checkFragments)...)](const FragmentList &fragments) {
    REQUIRE(size == fragments.size());
    applyFunctionTupleToVector(fragments, checkFragments);
  };
}

template <typename CheckBody, typename CheckFrame>
auto checkFunctionFragment(CheckBody &&checkBody, CheckFrame &&checkFrame) {
  return [checkBody = std::forward<CheckBody>(checkBody),
          checkFrame =
            std::forward<CheckFrame>(checkFrame)](const Fragment &fragment) {
    auto pFunctionFragment = get<FunctionFragment>(&fragment);
    REQUIRE(pFunctionFragment);
    checkBody(pFunctionFragment->m_body);
    checkFrame(pFunctionFragment->m_frame);
  };
}

auto checkStringFragment(const OptLabel &label, const std::string &string) {
  return [&label, &string](const Fragment &fragment) {
    auto pStringFragment = get<StringFragment>(&fragment);
    REQUIRE(pStringFragment);
    REQUIRE(label);
    REQUIRE(*label == pStringFragment->m_label);
    REQUIRE(string == pStringFragment->m_string);
  };
}

template <typename... CheckFormals>
auto checkFrame(const OptLabel &label, CheckFormals &&... checkFormals) {
  return [&label, size = sizeof...(checkFormals),
          checkFormals =
            std::forward_as_tuple(std::forward<CheckFormals>(checkFormals)...)](
           const std::shared_ptr<const frame::Frame> &frame) {
    REQUIRE(frame);
    REQUIRE(label == frame->name());
    REQUIRE(frame->formals().size() == size);
    applyFunctionTupleToVector(frame->formals(), checkFormals);
  };
}

auto checkFrameFormal(int offset) {
  return [offset](const frame::VariableAccess &varAccess) {
    auto pInFrame = get<frame::InFrame>(&varAccess);
    REQUIRE(pInFrame);
    REQUIRE(pInFrame->m_offset == offset);
  };
}

auto checkRegFormal(const OptReg &reg) {
  return [&reg](const frame::VariableAccess &varAccess) {
    auto pInReg = get<frame::InReg>(&varAccess);
    REQUIRE(pInReg);
    REQUIRE(reg);
    REQUIRE(pInReg->m_reg == *reg);
  };
}

auto checkStaticLinkFormal() { return checkFrameFormal(-WORD_SIZE); }

TEST_CASE("lvalue") {
  SECTION("identifier") {
    auto result = checkedCompile(R"(
let
  var i := 0
in
  i
end
)");
    OptReg reg;
    checkExpressionSequence(checkSequence( // declarations
                              checkMove( // allocates a register and set it to 0
                                checkReg(reg), checkInt(0))),
                            checkReg(reg) // return previously allocated reg
                            )(result.first);
  }
  SECTION("field") {
    auto result = checkedCompile(R"(
let
  type rec = {i : int}
  var r : rec := nil
in
  r.i
end
)");
    OptReg reg;
    checkExpressionSequence(checkSequence( // declarations
                              checkNop(),  // type declarations are no-ops
                              checkMove( // allocates a register and set it to 0
                                checkReg(reg), checkInt(0))),
                            checkMemberAccess(reg, 0))(result.first);
  }
  SECTION("array element") {
    auto result = checkedCompile(R"(
let
  type arr = array of int
  var a := arr[2] of 3
in
  a[1]
end
)");
    OptReg reg1, reg2;
    checkExpressionSequence(
      checkSequence(              // declarations
        checkNop(),               // type declarations are no-ops,
        checkMove(checkReg(reg1), // move reg2 to reg1,
                  checkExpressionSequence(
                    checkSequence(
                      checkMove( // set reg2 to result of malloc(array size),
                        checkReg(reg2),
                        checkExternalCall(
                          "malloc", checkBinaryOperation(ir::BinOp::MUL,
                                                         checkInt(WORD_SIZE),
                                                         checkInt(2)))),
                      checkExpressionStatement(
                        checkExternalCall( // call initArray(array size,
                                           // array init val)
                          "initArray", checkInt(2), checkInt(3)))),
                    checkReg(reg2)))),
      checkMemberAccess(reg1, 1))(result.first);
  }
}

TEST_CASE("nil") {
  auto result = checkedCompile("nil");
  checkInt(0) // same as 0
    (result.first);
}

TEST_CASE("sequence") {
  // the last expression is the sequence results, the previous ones are just
  // statements
  SECTION("empty") {
    auto result = checkedCompile("()");
    checkInt(0)(result.first);
  }

  SECTION("single") {
    auto result = checkedCompile("(42)");
    checkInt(42)(result.first);
  }

  SECTION("multiple") {
    auto result = checkedCompile(R"((1;"two";flush()))");
    OptLabel stringLabel, flush = temp::Label{"flush"};
    checkExpressionSequence(
      checkSequence(checkExpressionStatement(checkInt(1)),
                    checkExpressionStatement(checkString(stringLabel))),
      checkLocalCall(flush))(result.first);
    checkFragments(checkStringFragment(stringLabel, "two"))(result.second);
  }
}

TEST_CASE("integer") {
  auto result = checkedCompile("42");
  checkInt(42)(result.first);
}

TEST_CASE("string") {
  auto str    = R"("\tHello \"World\"!\n")";
  auto result = checkedCompile(str);
  OptLabel stringLabel;
  checkString(stringLabel)(result.first);
  checkFragments(checkStringFragment(stringLabel, "\tHello \"World\"!\n"))(
    result.second);
}

TEST_CASE("function call") {
  OptReg rv = reg(Registers::RAX);
  OptLabel functionLabel;

  SECTION("with no arguments") {
    auto result = checkedCompile(R"(
let
  function f() = ()
in
  f()
end
)");
    checkExpressionSequence(checkSequence( // declarations
                              checkNop()   // function declaration
                              ),
                            checkLocalCall(functionLabel) // call
                            )(result.first);
    checkFragments(checkFunctionFragment(
      checkSequence(               // f
        checkLabel(functionLabel), // function label
        checkMove(                 // body
          checkReg(rv), checkInt(0))),
      checkFrame(functionLabel, checkStaticLinkFormal())))(result.second);
  }

  SECTION("with arguments") {
    OptReg rcx  = reg(Registers::RCX);
    auto result = checkedCompile(R"(
let
  function f(a: int) = ()
in
  f(2)
end
)");
    checkExpressionSequence(checkSequence( // declarations
                              checkNop()   // function declaration
                              ),
                            checkLocalCall( // call
                              functionLabel, checkInt(2)))(result.first);
    checkFragments(checkFunctionFragment(
      checkSequence(               // f
        checkLabel(functionLabel), // function label
        checkMove(                 // body
          checkReg(rv), checkInt(0))),
      checkFrame(functionLabel, checkStaticLinkFormal(), checkRegFormal(rcx))))(
      result.second);
  }
}

TEST_CASE("record") {
  SECTION("empty") {
    auto result = checkedCompile(R"(
let 
  type t = {}
in
  t{}
end
)");
    OptReg r;
    checkExpressionSequence(
      checkSequence( // declarations
        checkNop()   // type declaration
        ),
      checkExpressionSequence(
        checkSequence(checkMove( // move result of malloc(record_size) to r
          checkReg(r), checkExternalCall("malloc", checkInt(0)))),
        checkReg(r)))(result.first);
  }

  SECTION("not empty") {
    auto result = checkedCompile(R"(
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
        checkNop()   // type declaration
        ),
      checkExpressionSequence(
        checkSequence(
          checkMove( // move result of malloc(record_size) to r
            checkReg(r), checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
          checkMove( // init first member with 2
            checkMemberAccess(r, 0), checkInt(2)),
          checkMove( // init second member with "hello"
            checkMemberAccess(r, 1), checkString(stringLabel))),
        checkReg(r)))(result.first);
    checkFragments(checkStringFragment(stringLabel, "hello"))(result.second);
  }
}

TEST_CASE("array") {
  auto result = checkedCompile(R"(
let
  type t = array of int
in
  t[2] of 3
end
)");
  OptReg r;
  checkExpressionSequence(
    checkSequence( // declarations
      checkNop()   // type declaration
      ),
    checkExpressionSequence(
      checkSequence(
        checkMove( // move result of malloc(array_size) to r
          checkReg(r),
          checkExternalCall("malloc", checkBinaryOperation(ir::BinOp::MUL,
                                                           checkInt(WORD_SIZE),
                                                           checkInt(2)))),
        checkExpressionStatement( // call initArray(array_size, init_val)
          checkExternalCall("initArray", checkInt(2), checkInt(3)))),
      checkReg(r)))(result.first);
}

TEST_CASE("arithmetic") {
  static const std::pair<std::string, ir::BinOp> operations[] = {
    {"+", ir::BinOp::PLUS},
    {"-", ir::BinOp::MINUS},
    {"*", ir::BinOp::MUL},
    {"/", ir::BinOp::DIV}};

  for (const auto &operation : operations) {
    SECTION(Catch::StringMaker<ir::BinOp>::convert(operation.second)) {
      auto result = checkedCompile(boost::str(boost::format(R"(
  let
    var i : int := 2 %1% 3
  in
    i
  end
  )") % operation.first));
      OptReg r;
      checkExpressionSequence(
        checkSequence( // declarations
          checkMove(
            checkReg(r), // move result of 2 op 3 to a r
            checkBinaryOperation(operation.second, checkInt(2), checkInt(3)))),
        checkReg(r) // return i
        )(result.first);
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
        auto result = checkedCompile(boost::str(boost::format(R"(
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
                checkSequence(checkMove(checkReg(r),
                                        checkInt(1)), // move 1 to r
                              checkConditionalJump(
                                relation.second, checkInt(2), checkInt(3),
                                trueDest, &falseDest), // check condition,
                                                       // jumping to trueDest
                                                       // or falseDest
                                                       // accordingly
                              checkLabel(falseDest),
                              checkMove(checkReg(r),
                                        checkInt(0)), // if condition is false,
                                                      // move 0 to r
                              checkLabel(trueDest)),
                checkReg(r) // move r to i
                ))),
          checkReg(i) // return i
          )(result.first);
      }
    }
  }

  SECTION("string") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto result = checkedCompile(boost::str(boost::format(R"(
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
                    checkExternalCall("stringCompare", checkString(leftString),
                                      checkString(rightString)),
                    checkInt(0), trueDest,
                    &falseDest), // check condition,
                                 // jumping to trueDest
                                 // or falseDest
                                 // accordingly
                  checkLabel(falseDest),
                  checkMove(checkReg(r2),
                            checkInt(0)), // if condition is false,
                                          // move 0 to r2
                  checkLabel(trueDest)),
                checkReg(r2) // move r2 to r1
                ))),
          checkReg(r1) // return i
          )(result.first);
        checkFragments(checkStringFragment(leftString, "2"),
                       checkStringFragment(rightString, "3"))(result.second);
      }
    }
  }

  static const std::pair<std::string, ir::RelOp> eqNeq[] = {
    {"=", ir::RelOp::EQ}, {"<>", ir::RelOp::NE}};

  SECTION("record") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto result = checkedCompile(boost::str(boost::format(R"(
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
          checkSequence( // declarations
            checkNop(),  // type declaration
            checkMove(   // move record to i
              checkReg(i),
              checkExpressionSequence(
                checkSequence(checkMove( // move result of
                                         // malloc(record_size) to r
                  checkReg(r1), checkExternalCall("malloc", checkInt(0)))),
                checkReg(r1)))),
          checkExpressionSequence(
            checkSequence(
              checkMove(checkReg(r2),
                        checkInt(1)), // move 1 to r2
              checkConditionalJump(relation.second, checkReg(i), checkReg(i),
                                   trueDest,
                                   &falseDest), // check condition,
                                                // jumping to trueDest
                                                // or falseDest
                                                // accordingly
              checkLabel(falseDest),
              checkMove(checkReg(r2),
                        checkInt(0)), // if condition is false, move 0 to r2
              checkLabel(trueDest)),
            checkReg(r2) // move r2 to r1
            ))(result.first);
      }
    }
  }

  SECTION("array") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto result = checkedCompile(boost::str(boost::format(R"(
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
          checkSequence( // declarations
            checkNop(),  // type declaration
            checkMove(   // move array to i
              checkReg(i),
              checkExpressionSequence(
                checkSequence(
                  checkMove( // move result of malloc(array_size) to
                             // r1
                    checkReg(r1),
                    checkExternalCall("malloc",
                                      checkBinaryOperation(ir::BinOp::MUL,
                                                           checkInt(WORD_SIZE),
                                                           checkInt(2)))),
                  checkExpressionStatement( // call
                                            // initArray(array_size,
                                            // init_val)
                    checkExternalCall("initArray", checkInt(2), checkInt(3)))),
                checkReg(r1)))),
          checkExpressionSequence(
            checkSequence(
              checkMove(checkReg(r2),
                        checkInt(1)), // move 1 to r2
              checkConditionalJump(relation.second, checkReg(i), checkReg(i),
                                   trueDest,
                                   &falseDest), // check condition,
                                                // jumping to trueDest
                                                // or falseDest
                                                // accordingly
              checkLabel(falseDest),
              checkMove(checkReg(r2),
                        checkInt(0)), // if condition is false, move 0 to r2
              checkLabel(trueDest)),
            checkReg(r2) // move r2 to r1
            ))(result.first);
      }
    }
  }
}

TEST_CASE("boolean") {
  SECTION("and") {
    auto result = checkedCompile("2 & 3");
    // a & b is translated to if a then b else 0
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkExpressionSequence(
      checkSequence(
        checkConditionalJump( // test
          ir::RelOp::NE, checkInt(2), checkInt(0), trueDest, &falseDest),
        checkSequence(                                               // then
          checkLabel(trueDest), checkMove(checkReg(r), checkInt(3)), // 3
          checkJump(checkLabel(joinDest))),
        checkSequence(                                                // else
          checkLabel(falseDest), checkMove(checkReg(r), checkInt(0)), // 0
          checkJump(checkLabel(joinDest))),
        checkLabel(joinDest) // branches join here
        ),
      checkReg(r))(result.first);
  }

  SECTION("or") {
    auto result = checkedCompile("2 | 3");
    // a | b is translated to if a then 1 else b
    OptReg r;
    OptLabel trueDest, falseDest, joinDest;
    checkExpressionSequence(
      checkSequence(
        checkConditionalJump( // test
          ir::RelOp::NE, checkInt(2), checkInt(0), trueDest, &falseDest),
        checkSequence(                                               // then
          checkLabel(trueDest), checkMove(checkReg(r), checkInt(1)), // 1
          checkJump(checkLabel(joinDest))),
        checkSequence(                                                // else
          checkLabel(falseDest), checkMove(checkReg(r), checkInt(3)), // 3
          checkJump(checkLabel(joinDest))),
        checkLabel(joinDest) // branches join here
        ),
      checkReg(r))(result.first);
  }
}

TEST_CASE("assignment") {
  auto result = checkedCompile(R"(
let
  var a := 3
  var b := 0
in
  b := a
end
)");
  OptReg a, b;
  checkExpressionSequence(checkSequence( // declarations
                            checkMove(   // move 3 to a
                              checkReg(a), checkInt(3)),
                            checkMove( // move 0 to b
                              checkReg(b), checkInt(0))),
                          checkStatementExpression(checkMove( // move b to a
                            checkReg(b), checkReg(a))))(result.first);
}

TEST_CASE("if then") {
  SECTION("no else") {
    auto result = checkedCompile(R"(
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
        checkMove(   // move 0 to a
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
        )))(result.first);
  }

  SECTION("with else") {
    SECTION("both expression are of type void") {
      auto result = checkedCompile(R"(
let
  var a := 0
in
  if 0 then a := 3 else a := 4
end
)");
      OptReg a;
      OptLabel trueDest, falseDest, joinDest;
      checkExpressionSequence(
        checkSequence( // declarations
          checkMove(   // move 0 to a
            checkReg(a), checkInt(0))),
        checkStatementExpression(checkSequence(
          checkConditionalJump( // test
            ir::RelOp::NE, checkInt(0), checkInt(0), trueDest, &falseDest),
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
          )))(result.first);
    }

    SECTION("both expression are of the same type") {
      auto result = checkedCompile(R"(
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
          checkMove(checkReg(a),
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
        checkReg(a)                   // return a
        )(result.first);
    }
  }
}

TEST_CASE("while") {
  auto result = checkedCompile(R"(
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
      checkMove(   // move 2 to b
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
            checkConditionalJump(ir::RelOp::GT, checkReg(b), checkInt(0),
                                 trueDest,
                                 &falseDest), // check condition,
                                              // jumping to trueDest
                                              // or falseDest
                                              // accordingly
            checkLabel(falseDest),
            checkMove(checkReg(r),
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
      checkLabel(loopDone))))(result.first);
}

TEST_CASE("for") {
  auto result = checkedCompile(R"(
let
  function f(a: int) = ()
in
  for a := 0 to 4 do f(a)
end
)");
  OptReg rv, a, limit, rcx = reg(Registers::RCX);
  OptLabel functionLabel, loopStart, loopDone;
  checkExpressionSequence(
    checkSequence( // declarations
      checkNop()   // function declaration
      ),
    checkStatementExpression(checkSequence( // for
      checkMove(                            // move 0 to a
        checkReg(a), checkInt(0)),
      checkMove( // move 4 to limit
        checkReg(limit), checkInt(4)),
      checkConditionalJump( // skip the loop if a > limit
        ir::RelOp::GT, checkReg(a), checkReg(limit), loopDone),
      checkLabel(loopStart),
      checkExpressionStatement( // body
        checkLocalCall(         // f(a)
          functionLabel, checkReg(a))),
      checkMove( // a := a + 1
        checkReg(a),
        checkBinaryOperation(ir::BinOp::PLUS, checkReg(a), checkInt(1))),
      checkConditionalJump(ir::RelOp::LT, checkReg(a), checkReg(limit),
                           loopStart),
      checkLabel(loopDone))))(result.first);
  checkFragments(checkFunctionFragment(
    checkSequence(               // f
      checkLabel(functionLabel), // function label
      checkMove(                 // body
        checkReg(rv), checkInt(0))),
    checkFrame(functionLabel, checkStaticLinkFormal(), checkRegFormal(rcx))))(
    result.second);
}

TEST_CASE("break") {
  SECTION("for") {
    auto result = checkedCompile("for a := 0 to 2 do break");
    OptReg a, limit;
    OptLabel functionLabel, loopStart, loopDone;
    checkStatementExpression(checkSequence( // for
      checkMove(                            // move 0 to a
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
      checkLabel(loopDone)))(result.first);
  }

  SECTION("while") {
    auto result = checkedCompile("while 1 do break");
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
      checkLabel(loopDone)))(result.first);
  }
}

TEST_CASE("type declarations") {
  SECTION("name") {
    auto result = checkedCompile(R"(
let
  type a = int
  var b : a := 0
in
end
)");
    OptReg b;
    checkStatementExpression(checkSequence( // declarations
      checkNop(),                           // type declarations are no-ops
      checkMove(                            // move 0 to b
        checkReg(b), checkInt(0))))(result.first);
  }

  SECTION("record") {
    auto result = checkedCompile(R"(
let
  type rec = {i : int}
  var r := rec{i=2}
in
end
)");
    OptReg r1, r2;
    checkStatementExpression(checkSequence( // declarations
      checkNop(),                           // type declarations are no-ops
      checkMove(                            // move r2 to r1
        checkReg(r1),
        checkExpressionSequence(
          checkSequence(
            checkMove( // move result of malloc(record_size) to r2
              checkReg(r2), checkExternalCall("malloc", checkInt(WORD_SIZE))),
            checkMove( // init first member with 2
              checkMemberAccess(r2, 0), checkInt(2))),
          checkReg(r2)))))(result.first);
  }

  SECTION("array") {
    auto result = checkedCompile(R"(
let
  type arr = array of string
  var a := arr[3] of "a"
in
end
)");
    OptReg r1, r2;
    OptLabel stringLabel;
    checkStatementExpression(checkSequence( // declarations
      checkNop(),                           // type declarations are no-ops,
      checkMove(
        checkReg(r1), // move reg2 to reg1,
        checkExpressionSequence(
          checkSequence(
            checkMove( // set reg2 to result of malloc(array size),
              checkReg(r2),
              checkExternalCall(
                "malloc", checkBinaryOperation(
                            ir::BinOp::MUL, checkInt(WORD_SIZE), checkInt(3)))),
            checkExpressionStatement(checkExternalCall( // call initArray(array
                                                        // size, array init val)
              "initArray", checkInt(3), checkString(stringLabel)))),
          checkReg(r2)))))(result.first);
    checkFragments(checkStringFragment(stringLabel, "a"))(result.second);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto result = checkedCompile(R"(
let
  type intlist = {hd: int, tl: intlist}
in
  intlist{hd = 3, tl = intlist{hd = 4, tl = nil}}
end
)");
      OptReg r1, r2;
      checkExpressionSequence(
        checkSequence( // declarations
          checkNop()   // type declaration
          ),
        checkExpressionSequence( // outer record
          checkSequence(
            checkMove( // move result of malloc(record_size) to r1
              checkReg(r1),
              checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
            checkMove( // init first member
              checkMemberAccess(r1, 0), checkInt(3)),
            checkMove( // init second member
              checkMemberAccess(r1, 1),
              checkExpressionSequence( // inner record
                checkSequence(
                  checkMove( // move result of malloc(record_size)
                             // to r2
                    checkReg(r2),
                    checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                  checkMove( // init first member
                    checkMemberAccess(r2, 0), checkInt(4)),
                  checkMove( // init second member
                    checkMemberAccess(r2, 1), checkInt(0))),
                checkReg(r2)))),
          checkReg(r1)))(result.first);
    }

    SECTION("mutually") {
      auto result = checkedCompile(R"(
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
          checkNop()   // type declaration
          ),
        checkExpressionSequence( // outer record
          checkSequence(
            checkMove( // move result of malloc(record_size) to r1
              checkReg(r1),
              checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
            checkMove( // init first member
              checkMemberAccess(r1, 0), checkInt(1)),
            checkMove( // init second member
              checkMemberAccess(r1, 1),
              checkExpressionSequence( // inner record
                checkSequence(
                  checkMove( // move result of malloc(record_size)
                             // to r2
                    checkReg(r2),
                    checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                  checkMove( // init first member
                    checkMemberAccess(r2, 0),
                    checkExpressionSequence( // most inner record
                      checkSequence(
                        checkMove( // move result of
                                   // malloc(record_size) to
                                   // r3
                          checkReg(r3),
                          checkExternalCall("malloc", checkInt(2 * WORD_SIZE))),
                        checkMove( // init first member
                          checkMemberAccess(r3, 0), checkInt(2)),
                        checkMove( // init second member
                          checkMemberAccess(r3, 1), checkInt(0))),
                      checkReg(r3))),
                  checkMove( // init second member
                    checkMemberAccess(r2, 1), checkInt(0))),
                checkReg(r2)))),
          checkReg(r1)))(result.first);
    }
  }
}

TEST_CASE("function declarations") {
  OptReg rv = reg(Registers::RAX);

  SECTION("simple") {
    auto result = checkedCompile(R"(
let
  function f() = ()
in
end
)");
    OptLabel functionLabel;
    checkStatementExpression(checkSequence( // declarations
      checkNop()                            // function declaration
      ))(result.first);
    checkFragments(checkFunctionFragment(
      checkSequence(               // f
        checkLabel(functionLabel), // function label
        checkMove(                 // body
          checkReg(rv), checkInt(0))),
      checkFrame(functionLabel, checkStaticLinkFormal())))(result.second);
  }

  SECTION("with parameters") {
    auto result = checkedCompile(R"(
let
  function f(a:int, b:string) = ()
in
end
)");
    OptLabel functionLabel;
    OptReg rcx = reg(Registers::RCX), rdx = reg(Registers::RDX);
    checkStatementExpression(checkSequence( // declarations
      checkNop()                            // function declaration
      ))(result.first);
    checkFragments(checkFunctionFragment(
      checkSequence(               // f
        checkLabel(functionLabel), // function label
        checkMove(                 // body
          checkReg(rv), checkInt(0))),
      checkFrame(functionLabel, checkStaticLinkFormal(), checkRegFormal(rcx),
                 checkRegFormal(rdx))))(result.second);
  }

  SECTION("with return value") {
    auto result = checkedCompile(R"(
let
  function f(a:int, b:string) : string = (a:=1;b)
in
end
)");
    OptReg a, b;
    OptLabel functionLabel;
    OptReg rcx = reg(Registers::RCX), rdx = reg(Registers::RDX);
    checkStatementExpression(checkSequence( // declarations
      checkNop()                            // function declaration
      ))(result.first);
    checkFragments(checkFunctionFragment(
      checkSequence(               // f
        checkLabel(functionLabel), // function label
        checkMove(                 // body
          checkReg(rv),
          checkExpressionSequence(checkSequence(checkMove( // a=1
                                    checkReg(a), checkInt(1))),
                                  checkReg(b)))),
      checkFrame(functionLabel, checkStaticLinkFormal(), checkRegFormal(rcx),
                 checkRegFormal(rdx))))(result.second);
  }

  SECTION("recursive") {
    SECTION("single") {
      auto result = checkedCompile(R"(
let
  function fib(n:int) : int = (
    if (n = 0 | n = 1) then n else fib(n-1)+fib(n-2)
  )
in
end
)");
      CAPTURE(result.first);
      OptReg rcx = reg(Registers::RCX), n, r[3];
      OptLabel functionLabel, trueDest[3], falseDest[3], joinDest[3];
      checkStatementExpression(checkSequence( // declarations
        checkNop()                            // function declaration
        ))(result.first);
      checkFragments(checkFunctionFragment(
        checkSequence(               // f
          checkLabel(functionLabel), // function label
          checkMove(                 // body
            checkReg(rv),
            checkExpressionSequence(
              checkSequence(
                checkConditionalJump( // test
                  ir::RelOp::NE,
                  checkExpressionSequence(
                    checkSequence(
                      checkConditionalJump( // n = 0
                        ir::RelOp::EQ, checkReg(n), checkInt(0), trueDest[0],
                        &falseDest[0]),
                      checkSequence( // then
                        checkLabel(trueDest[0]),
                        checkMove(checkReg(r[0]), checkInt(1)),
                        checkJump(checkLabel(joinDest[0]))),
                      checkSequence( // else
                        checkLabel(falseDest[0]),
                        checkMove(checkReg(r[0]),
                                  checkExpressionSequence( // n
                                                           // =
                                                           // 1
                                    checkSequence(
                                      checkMove(checkReg(r[1]), checkInt(1)),
                                      checkConditionalJump(
                                        ir::RelOp::EQ, checkReg(n), checkInt(1),
                                        trueDest[1], &falseDest[1]),
                                      checkLabel(falseDest[1]),
                                      checkMove(checkReg(r[1]), checkInt(0)),
                                      checkLabel(trueDest[1])),
                                    checkReg(r[1]))),
                        checkJump(checkLabel(joinDest[0]))),
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
                      checkCall(checkLabel(functionLabel), checkStaticLink<2>(),
                                checkBinaryOperation(ir::BinOp::MINUS,
                                                     checkReg(n), checkInt(1))),
                      checkCall(checkLabel(functionLabel), checkStaticLink<2>(),
                                checkBinaryOperation(ir::BinOp::MINUS,
                                                     checkReg(n),
                                                     checkInt(2))))),
                  checkJump(checkLabel(joinDest[2]))),
                checkLabel(joinDest[2])),
              checkReg(r[2])))),
        checkFrame(functionLabel, checkStaticLinkFormal(),
                   checkRegFormal(rcx))))(result.second);
    }

    SECTION("mutually") {
      auto result = checkedCompile(R"(
let
  function f() = (g())
  function g() = (h())
  function h() = (f())
in
end
)");

      OptLabel f, h, g;
      checkStatementExpression(checkSequence( // declarations
        checkNop()                            // function declaration
        ))(result.first);
      checkFragments(
        checkFunctionFragment(
          checkSequence( // f
            checkLabel(f),
            checkMove(checkReg(rv),
                      checkCall(checkLabel(g), checkStaticLink<2>()))),
          checkFrame(f, checkStaticLinkFormal())),
        checkFunctionFragment(
          checkSequence( // g
            checkLabel(g),
            checkMove(checkReg(rv),
                      checkCall(checkLabel(h), checkStaticLink<2>()))),
          checkFrame(g, checkStaticLinkFormal())),
        checkFunctionFragment(
          checkSequence( // h
            checkLabel(h),
            checkMove(checkReg(rv),
                      checkCall(checkLabel(f), checkStaticLink<2>()))),
          checkFrame(h, checkStaticLinkFormal())))(result.second);
    }
  }

  SECTION("nested") {
    auto result = checkedCompile(R"(
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
    CAPTURE(result.first);
    OptReg fp = reg(Registers::RBP), rcx = reg(Registers::RCX);
    OptLabel f, g;
    checkExpressionSequence( // outer let
      checkSequence(         // declarations
        checkNop()           // function declaration
        ),
      checkLocalCall( // f(3)
        f, checkInt(3)))(result.first);
    checkFragments(
      checkFunctionFragment(
        checkSequence( // g
          checkLabel(g), checkMove(checkReg(rv),
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
                                     checkInt(2)))),
        checkFrame(g, checkStaticLinkFormal())),
      checkFunctionFragment(
        checkSequence( // f
          checkLabel(f), checkMove(checkReg(rv),
                                   checkExpressionSequence( // inner let
                                     checkSequence(         // declarations
                                       checkNop() // function declaration

                                       ),
                                     checkLocalCall(g)))),
        checkFrame(f, checkStaticLinkFormal(),
                   checkFrameFormal(2 * -WORD_SIZE))))(result.second);
  }
}

TEST_CASE("standard library") {
  auto checkVoidLibraryCall =
    [](const std::string name, const std::string params, auto &&... checkArgs) {
      OptLabel label = temp::Label{name};
      auto result    = checkedCompile(name + "(" + params + ")");
      checkLocalCall(label, std::forward<std::decay_t<decltype(checkArgs)>>(
                              checkArgs)...)(result.first);
      return result.second;
    };

  auto checkNonVoidLibraryCall =
    [](const std::string returnType, const std::string name,
       const std::string params, auto &&... checkArgs) {
      OptReg v;
      OptLabel label = temp::Label{name};
      auto result = checkedCompile("let var v : " + returnType + " := " + name
                                   + "(" + params + ") in end");
      checkStatementExpression(checkSequence( // declarations
        checkMove(
          checkReg(v),
          checkLocalCall(label, std::forward<std::decay_t<decltype(checkArgs)>>(
                                  checkArgs)...))))(result.first);
      return result.second;
    };

  auto emptyString = "\"\"";

  // function print(s : string)
  SECTION("print") {
    OptLabel stringLabel;
    auto fragments =
      checkVoidLibraryCall("print", emptyString, checkString(stringLabel));
    checkFragments(checkStringFragment(stringLabel, ""))(fragments);
  }

  // function flush()
  SECTION("flush") { checkVoidLibraryCall("flush", ""); }

  // function getchar() : string
  SECTION("getchar") { checkNonVoidLibraryCall("string", "getchar", ""); }

  // function ord(s: string) : int
  SECTION("ord") {
    OptLabel stringLabel;
    auto fragments = checkNonVoidLibraryCall("int", "ord", emptyString,
                                             checkString(stringLabel));
    checkFragments(checkStringFragment(stringLabel, ""))(fragments);
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
    auto fragments = checkNonVoidLibraryCall(
      "string", "substring", "\"\", 1, 2", checkString(stringLabel),
      checkInt(1), checkInt(2));
    checkFragments(checkStringFragment(stringLabel, ""))(fragments);
  }

  // function concat(s1: string, s2: string) : string
  SECTION("concat") {
    OptLabel stringLabel[2];
    auto fragments = checkNonVoidLibraryCall("string", "concat", "\"\", \"\"",
                                             checkString(stringLabel[0]),
                                             checkString(stringLabel[1]));
    checkFragments(checkStringFragment(stringLabel[0], ""),
                   checkStringFragment(stringLabel[1], ""))(fragments);
  }

  // function not(i : integer) : integer
  SECTION("not") { checkNonVoidLibraryCall("int", "not", "3", checkInt(3)); }

  // function exit(i: int)
  SECTION("exit") { checkVoidLibraryCall("exit", "5", checkInt(5)); }
}
