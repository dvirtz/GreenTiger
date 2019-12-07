#include "m68kCodeGenerator.h"
#include "Tree.h"
#include "m68kCallingConvention.h"
#include "m68kRegisters.h"
#include "variantMatch.h"
#include <range/v3/view/transform.hpp>

namespace tiger {
namespace assembly {
namespace m68k {

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention) :
    assembly::CodeGenerator{
      callingConvention,
      {
        // clang-format off
            Pattern{ir::Move{callingConvention.returnValue(), ir::Call{label()}}, Operation{"JSR `l0"}},
            Pattern{ir::Move{reg(), imm()}, Move{"MOVE #`i0, `s0"}},
            Pattern{ir::Move{reg(), label()}, Move{"MOVE #`l0, `s0"}},
            Pattern{ir::Move{ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}, imm()}, 
                Move{"MOVE #`i1, (#`i0, `s0)"}},
            Pattern{ir::Move{reg(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                Move{"MOVE (#`i0, `s1), `s0"}},
            Pattern{ir::Jump{label()}, Operation{"BRA `l0"}},
            Pattern{ir::BinaryOperation{ir::BinOp::MUL, exp(), exp()}, 
                Operation{"MOVE `s0, `d0", "MULS.L `s1, `d0"}},
            Pattern{ir::BinaryOperation{ir::BinOp::PLUS, exp(), exp()}, 
                Operation{"MOVE `s0, `d0", "ADD `s1, `d0"}},
            Pattern{ir::BinaryOperation{ir::BinOp::MINUS, exp(), exp()}, 
                Operation{"MOVE `s0, `d0", "SUB `s1, `d0"}},
            Pattern{ir::BinaryOperation{ir::BinOp::DIV, exp(), exp()}, 
                Operation{"MOVE `s0, `d0", "DIVS.L `s1, `d0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::EQ, exp(), exp()},
                Operation{"SUB `s0, `s1", "BEQ `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::NE, exp(), exp()},
                Operation{"SUB `s0, `s1", "BNE `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::LT, exp(), exp()},
                Operation{"SUB `s0, `s1", "BLT `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::GT, exp(), exp()},
                Operation{"SUB `s0, `s1", "BGT `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::LE, exp(), exp()},
                Operation{"SUB `s0, `s1", "BLE `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::GE, exp(), exp()},
                Operation{"SUB `s0, `s1", "BGE `l0"}},
            Pattern{ir::Move{reg(), reg()}, Move{"MOVE `s1, `s0"}},
            Pattern{ir::Move{reg(), exp()}, Move{"MOVE `s1, `s0"}},
            Pattern{ir::Move{exp(), imm()}, Move{"MOVE #`i0, `s0"}},
            Pattern{ir::Move{exp(), label()}, Move{"MOVE #`l0, `s0"}},
            Pattern{ir::Move{ir::MemoryAccess{reg()}, reg()}, Move{"MOVE `s1, (`s0)"}},
            Pattern{ir::Move{exp(), exp()}, Move{"MOVE `s1, `s0"}},
            Pattern{ir::MemoryAccess{exp()}, Move{"MOVE (`s0), `d0"}},
            Pattern{ir::Expression{imm()}, Move{"MOVE #`i0, `d0"}},
            Pattern{ir::Call{label()}, Operation{"JSR `l0"}}, 
            Pattern{ir::Call{exp()}, Operation{"JSR `s0"}},
            Pattern{ir::Statement{label()}, Label{"`l0:"}}
        // clang-format on
      }} {}

Instructions CodeGenerator::translateString(const temp::Label &label,
                                            const std::string &string,
                                            temp::Map & /* tempMap */) {
  return {Label{"`l0:", label}, Operation{{".string" + escape(string)}}};
}

Instructions
  CodeGenerator::translateArgs(const std::vector<ir::Expression> &args,
                               const temp::Map & /* tempMap */) const {
  return args | ranges::views::transform([](const ir::Expression &arg) {
           return helpers::match(arg)(
             [](int i) {
               return Move{"MOVE #`i0, -(sp)", {}, {}, {}, {i}};
             },
             [](const temp::Register &reg) {
               return Move{"MOVE `s0, -(sp)", {}, {reg}};
             },
             [](const temp::Label &label) {
               return Move{"MOVE #`l0, -(sp)", {}, {}, {label}};
             },
             [](const auto & /*default*/) {
               assert(false && "Unsupported arg type");
               return Move{};
             });
         });
}

} // namespace m68k
} // namespace assembly
} // namespace tiger
