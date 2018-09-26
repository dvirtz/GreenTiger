#include "Test.h"

TEST_CASE("comparison") {
  static const std::pair<std::string, ir::RelOp> relations[] = {
    {"=", ir::RelOp::EQ},  {"<>", ir::RelOp::NE}, {"<", ir::RelOp::LT},
    {"<=", ir::RelOp::LE}, {">", ir::RelOp::GT},  {">=", ir::RelOp::GE}};
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
        OptReg regs[4];
        OptLabel trueDest, falseDest;
        checkProgram(
          program,
          checkMove(checkReg(regs[0]), checkImm(1)) > // move 1 to r
            checkMove(checkReg(regs[1]), checkImm(2))
            > checkMove(checkReg(regs[2]), checkImm(3))
            > checkConditionalJump(relation.second, regs[1], regs[2], trueDest)
            > // check condition, jumping to trueDest or falseDest accordingly
            checkLabel(falseDest) > ':' > checkMove(checkReg(regs[0]),
                                                    checkImm(0))
            > // if condition is false, move 0 to r
            checkLabel(trueDest) > ':' > checkMove(checkReg(regs[3]),
                                                   checkReg(regs[0]))
            >                                         // move r to i
            checkMove(returnReg(), checkReg(regs[3])) // return i
        );
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
        OptReg regs[4];
        OptLabel trueDest, falseDest;
        OptLabel leftString, rightString;
        checkProgram(
          program,
          checkMove(checkReg(regs[0]),
                    checkImm(1))
            > // move 1 to r2
            checkExternalCall("stringCompare",
                              checkArg(0, checkString(leftString))
                                > checkArg(1, checkString(rightString)))
            > checkMove(checkReg(regs[1]), returnReg())
            > checkMove(checkReg(regs[2]), checkImm(0)) >
            // stringCompare(s1, s2) returns -1, 0 or 1 when s1 is less than,
            // equal or greater than s2, respectively so s1 op s2 is the same as
            // stringCompare(s1, s2) op 0
            checkConditionalJump(relation.second, regs[1], regs[2], trueDest)
            > // check condition,
              // jumping to trueDest
              // or falseDest
              // accordingly
            checkLabel(falseDest) > ':' > checkMove(checkReg(regs[0]),
                                                    checkImm(0))
            > // if condition is false,
              // move 0 to r2
            checkLabel(trueDest) > ':'
            > checkMove(checkReg(regs[3]),
                        checkReg(regs[0]) // move r2 to r1
                        )
            > checkMove(returnReg(),
                        checkReg(regs[3]) // return i
                        ),
          checkStringInit(leftString, R"("2")")
            > checkStringInit(rightString, R"("3")"));
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
        OptReg regs[3];
        OptLabel trueDest, falseDest;
        checkProgram(
          program, checkExternalCall("malloc", checkArg(0, checkImm(0)))
                     > checkMove( // move result of
                                  // malloc(record_size) to r
                         checkReg(regs[0]), returnReg())
                     > checkMove( // move record to i
                         checkReg(regs[1]), checkReg(regs[0]))
                     > checkMove(checkReg(regs[2]),
                                 checkImm(1))
                     > // move 1 to r2
                     checkConditionalJump(relation.second, regs[1], regs[1],
                                          trueDest)
                     > // check condition,
                       // jumping to trueDest
                       // or falseDest
                       // accordingly
                     checkLabel(falseDest) > ':' > checkMove(checkReg(regs[2]),
                                                             checkImm(0))
                     > // if condition is false, move 0 to r2
                     checkLabel(trueDest) > ':'
                     > checkMove(returnReg(),
                                 checkReg(regs[2]) // return result
                                 ));
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
        OptReg regs[6];
        OptLabel trueDest, falseDest;
        checkProgram(
          program,
          checkMove(checkReg(regs[0]), checkImm(wordSize()))
            > checkMove(checkReg(regs[1]), checkImm(2))
            > checkBinaryOperation(ir::BinOp::MUL, checkReg(regs[0]),
                                   checkReg(regs[1]), regs[2])
            > checkExternalCall("malloc", checkArg(0, checkReg(regs[2])))
            > checkMove( // move result of malloc(array_size) to r
                checkReg(regs[3]), returnReg())
            > checkExternalCall("initArray", checkArg(0, checkImm(2))
                                               > checkArg(1, checkImm(3)))
            >          // call initArray(array_size, init_val)
            checkMove( // move array to i
              checkReg(regs[4]), checkReg(regs[3]))
            > checkMove(checkReg(regs[5]),
                        checkImm(1))
            > // move 1 to r2
            checkConditionalJump(relation.second, regs[4], regs[4],
                                 trueDest)
            > // check condition,
              // jumping to trueDest
              // or falseDest
              // accordingly
            checkLabel(falseDest) > ':' > checkMove(checkReg(regs[5]),
                                                    checkImm(0))
            > // if condition is false, move 0 to r2
            checkLabel(trueDest) > ':'
            > checkMove(returnReg(),
                        checkReg(regs[5]) // return result
                        ));
      }
    }
  }
}