#include "Test.h"
#include <boost/format.hpp>

TEST_CASE_METHOD(TestFixture, "comparison") {
  static const std::pair<std::string, ir::RelOp> relations[] = {
    {"=", ir::RelOp::EQ},  {"<>", ir::RelOp::NE}, {"<", ir::RelOp::LT},
    {"<=", ir::RelOp::LE}, {">", ir::RelOp::GT},  {">=", ir::RelOp::GE}};
  OptLabel end;
  OptIndex endIndex;

  SECTION("integer") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto results = checkedCompile(boost::str(boost::format(R"(
 let
   var i : int := 2 %1% 3
 in
   i
 end
 )") % relation.first));
        OptReg regs[4];
        OptLabel trueDest, falseDest;
        OptIndex trueDestIndex, falseDestIndex;
        checkProgram(
          results, checkMain(), checkMove(regs[0], 1), // move 1 to r
          checkMove(regs[1], 2, {regs[0]}), checkMove(regs[2], 3, {regs[0], regs[1]}),
          checkConditionalJump(
            relation.second, regs[1], regs[2], trueDest, trueDestIndex, falseDestIndex,
                               {regs[0]}), // check condition, jumping to
                             // trueDest or falseDest accordingly
          checkLabel(falseDest, falseDestIndex),
          checkMove(regs[0],
                    0), // if condition is false, move 0 to r
          checkLabel(trueDest, trueDestIndex),
          checkMove(regs[3],
                    regs[0]),              // move r to i
          checkMove(returnReg(), regs[3]), // return i
          branchToEnd(end, endIndex), checkFunctionExit());
      }
    }
  }

  SECTION("string") {
    for (const auto &relation : relations) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto results = checkedCompile(boost::str(boost::format(R"(
   let
     var i : int := "2" %1% "3"
   in
     i
   end
   )") % relation.first));
        OptReg regs[4];
        OptLabel trueDest, falseDest, leftString, rightString;
        OptIndex trueDestIndex, falseDestIndex, leftStringIndex,
          rightStringIndex;
        checkProgram(
          results, checkStringInit(leftString, R"("2")", leftStringIndex),
          checkStringInit(rightString, R"("3")", rightStringIndex), checkMain(),
          checkMove(regs[0],
                    1), // move 1 to r2
          checkCall("stringCompare", {regs[0], returnReg()}, leftString, rightString),
          checkMove(regs[1], returnReg(), {regs[0]}),
          checkMove(regs[2], 0, {regs[0], regs[1]}),
          // stringCompare(s1, s2) returns -1, 0 or 1 when s1 is less
          // than, equal or greater than s2, respectively so s1 op s2
          // is the same as stringCompare(s1, s2) op 0
          checkConditionalJump(relation.second, regs[1], regs[2], trueDest,
                               trueDestIndex, falseDestIndex,
                               {regs[0]}), // check condition,
                                                // jumping to trueDest
                                                // or falseDest
                                                // accordingly
          checkLabel(falseDest, falseDestIndex),
          checkMove(regs[0],
                    0), // if condition is false,
                        // move 0 to r2
          checkLabel(trueDest, trueDestIndex),
          checkMove(regs[3],
                    regs[0]), // move r2 to r1
          checkMove(returnReg(),
                    regs[3]), // return i
          branchToEnd(end, endIndex), checkFunctionExit());
      }
    }
  }

  static const std::pair<std::string, ir::RelOp> eqNeq[] = {
    {"=", ir::RelOp::EQ}, {"<>", ir::RelOp::NE}};

  SECTION("record") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto results = checkedCompile(boost::str(boost::format(R"(
  let
   type t = {}
   var i := t{}
  in
   i %1% i
  end
  )") % relation.first));
        OptReg regs[3];
        OptLabel trueDest, falseDest;
        OptIndex trueDestIndex, falseDestIndex;
        checkProgram(
          results, checkMain(), checkCall("malloc", {returnReg()}, 0),
          checkMove(regs[0], returnReg()), // move result of
                                           // malloc(record_size) to r
          checkMove(regs[1], regs[0]),     // move record to i
          checkMove(regs[2], 1, {regs[1]}), // move 1 to r2
          checkConditionalJump(relation.second, regs[1], regs[1], trueDest,
                               trueDestIndex,
                               falseDestIndex), // check condition,
                                                // jumping to trueDest
                                                // or falseDest
                                                // accordingly
          checkLabel(falseDest, falseDestIndex),
          checkMove(regs[2], 0), // if condition is false, move 0 to r2
          checkLabel(trueDest, trueDestIndex),
          checkMove(returnReg(), regs[2]), // return result
          branchToEnd(end, endIndex), checkFunctionExit());
      }
    }
  }

  SECTION("array") {
    for (const auto &relation : eqNeq) {
      SECTION(Catch::StringMaker<ir::BinOp>::convert(relation.second)) {
        auto results = checkedCompile(boost::str(boost::format(R"(
  let
   type t = array of int
   var i := t[2] of 3
  in
   i %1% i
  end
  )") % relation.first));
        OptReg regs[6];
        OptLabel trueDest, falseDest;
        OptIndex trueDestIndex, falseDestIndex;
        checkProgram(
          results, checkMain(), checkMove(regs[0], wordSize()),
          checkMove(regs[1], 2, {regs[0]}),
          checkBinaryOperation(ir::BinOp::MUL, regs[0], regs[1], regs[2]),
          checkCall("malloc", {returnReg()}, regs[2]),
          checkMove(regs[3],
                    returnReg()), // move result of malloc(array_size) to r
          checkCall("initArray", {regs[3]}, 2,
                    3),                // call initArray(array_size, init_val)
          checkMove(regs[4], regs[3]), // move array to i
          checkMove(regs[5], 1, {regs[4]}), // move 1 to r2
          checkConditionalJump(relation.second, regs[4], regs[4], trueDest,
                               trueDestIndex, falseDestIndex,
                               {regs[5]}), // check condition,
                                                // jumping to trueDest
                                                // or falseDest
                                                // accordingly
          checkLabel(falseDest, falseDestIndex),
          checkMove(regs[5],
                    0), // if condition is false, move 0 to r2
          checkLabel(trueDest, trueDestIndex),
          checkMove(returnReg(),
                    regs[5] // return result
                    ),
          branchToEnd(end, endIndex), checkFunctionExit());
      }
    }
  }
}