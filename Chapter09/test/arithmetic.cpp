#include "Test.h"

TEST_CASE("arithmetic") {
  static const std::pair<std::string, ir::BinOp> operations[] = {
    {"+", ir::BinOp::PLUS},
    {"-", ir::BinOp::MINUS},
    {"*", ir::BinOp::MUL},
    {"/", ir::BinOp::DIV}};

  for (const auto &op : operations) {
    SECTION(Catch::StringMaker<ir::BinOp>::convert(op.second)) {
      auto program = checkedCompile(boost::str(boost::format(R"(
 let
   var i : int := 2 %1% 3
 in
   i
 end
 )") % op.first));
      OptReg regs[4];
      checkProgram(
        program, checkMove(checkReg(regs[0]), checkImm(2))
                   > checkMove(checkReg(regs[1]), checkImm(3))
                   > checkBinaryOperation(op.second, checkReg(regs[0]),
                                          checkReg(regs[1]),
                                          regs[2])
                   > checkMove(checkReg(regs[3]), // move result of 2 op 3 to r
                               checkReg(regs[2]))
                   > checkMove( // return i
                       returnReg(), checkReg(regs[3])));
    }
  }
}