#include "Test.h"
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <boost/format.hpp>

TEST_CASE_METHOD(TestFixture, "arithmetic") {
  static const std::pair<std::string, ir::BinOp> operations[] = {
    {"+", ir::BinOp::PLUS},
    {"-", ir::BinOp::MINUS},
    {"*", ir::BinOp::MUL},
    {"/", ir::BinOp::DIV}};

  for (const auto &op : operations) {
    SECTION(Catch::StringMaker<ir::BinOp>::convert(op.second)) {
      auto results = checkedCompile(boost::str(boost::format(R"(
 let
   var i : int := 2 %1% 3
 in
   i
 end
 )") % op.first));
      OptReg regs[4];
      OptLabel end;
      OptIndex endIndex;
      checkProgram(results, checkMain(), checkMove(regs[0], 2),
                   checkMove(regs[1], 3, {regs[0]}),
                   checkBinaryOperation(op.second, regs[0], regs[1], regs[2]),
                   checkMove(regs[3], regs[2]), // move result of 2 op 3 to r
                   checkMove(returnReg(), regs[3]), // return i
                   branchToEnd(end, endIndex), checkFunctionExit());
    }
  }
}