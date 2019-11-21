#include "m68kCodeGenerator.h"
#include "Tree.h"
#include "m68kCallingConvention.h"
#include "variantMatch.h"
#include <range/v3/to_container.hpp>
#include <range/v3/view/transform.hpp>

namespace tiger {
namespace assembly {
namespace m68k {

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention) :
    assembly::CodeGenerator{
      callingConvention,
      {
        // clang-format off
            Pattern{ir::Move{ir::Call{label()}, callingConvention.returnValue()}, {{InstructionType::OPERATION, "JSR `l0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>}}},
            Pattern{ir::Move{imm(), reg()}, {{InstructionType::OPERATION, "MOVE #`i0, `d0", {0, 1}}}},
            Pattern{ir::Move{label(), reg()}, {{InstructionType::OPERATION, "MOVE #`l0, `d0", {0, 1}}}},
            Pattern{ir::Move{imm(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                {{InstructionType::OPERATION, "MOVE #`i0, (#`i1, `s0)", {0, 2, 1}}}},
            Pattern{ir::Move{reg(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                {{InstructionType::OPERATION, "MOVE #`s0, (#`i0, `s1)", {0, 2, 1}}}},
            Pattern{ir::Move{ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}, reg()}, 
                {{InstructionType::OPERATION, "MOVE (#`i0, `s0), `d0", {1, 0, 2}}}},
            Pattern{ir::Jump{label()}, {{InstructionType::JUMP, "BRA `l0", {0}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::MUL, exp(), exp()}, 
                {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 2}}, {InstructionType::OPERATION, "MULS.L `s0, `d0", {1, 2}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::PLUS, exp(), exp()}, 
                {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 2}}, {InstructionType::OPERATION, "ADD `s0, `d0", {1, 2}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::MINUS, exp(), exp()}, 
                {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 2}}, {InstructionType::OPERATION, "SUB `s0, `d0", {1, 2}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::DIV, exp(), exp()}, 
                {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 2}}, {InstructionType::OPERATION, "DIVS.L `s0, `d0", {1, 2}, {2}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::EQ, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BEQ `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::NE, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BNE `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::LT, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BLT `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::GT, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BGT `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::LE, exp(), exp(), label(), label()},
              { {InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BLE `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::GE, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "SUB `s0, `d0", {0, 1}, {1}}, {InstructionType::JUMP, "BGE `l0", {2, 3}}}},
            Pattern{ir::Move{reg(), reg()}, {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 1}}}},
            Pattern{ir::Move{exp(), reg()}, {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 1}}}},
            Pattern{ir::Move{imm(), exp()}, {{InstructionType::OPERATION, "MOVE #`i0, `d0", {0, 1}}}},
            Pattern{ir::Move{label(), exp()}, {{InstructionType::OPERATION, "MOVE #`l0, `d0", {0, 1}}}},
            Pattern{ir::Move{exp(), ir::MemoryAccess{exp()}}, {{InstructionType::OPERATION, "MOVE `s0, (`s1)", {0, 1}}}},
            Pattern{ir::Move{exp(), exp()}, {{InstructionType::MOVE, "MOVE `s0, `d0", {0, 1}}}},
            Pattern{ir::MemoryAccess{exp()}, {{InstructionType::OPERATION, "MOVE (`s0), `d0", {0, 1}}}},
            Pattern{ir::Expression{imm()}, {{InstructionType::OPERATION, "MOVE #`i0, `d0", {0, 1}}}},
            Pattern{ir::Call{label()}, {{InstructionType::OPERATION, "JSR `l0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>}}}, 
            Pattern{ir::Call{exp()}, {{InstructionType::OPERATION, "JSR `s0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>}}},
            Pattern{ir::Statement{label()}, {{InstructionType::LABEL, "`l0:", {0}}}}
        // clang-format on
      }} {}

Instructions CodeGenerator::translateString(const temp::Label &label,
                                            const std::string &string,
                                            temp::Map & /* tempMap */) {
  return {Label{"`l0:", label}, Operation{{".string " + escape(string)}}};
}

Instructions
  CodeGenerator::translateArgs(const std::vector<ir::Expression> &args,
                               const temp::Map & /* tempMap */) const {
  return args | ranges::view::transform([this](const ir::Expression &arg) {
           return helpers::match(arg)(
             [this](int i) {
               return Operation{"MOVE #`i0, +(`s0)",
                                {},
                                {m_callingConvention.stackPointer()},
                                {},
                                {i}};
             },
             [this](const temp::Register &reg) {
               return Operation{"MOVE `s0, +(`s1)",
                                {},
                                {reg, m_callingConvention.stackPointer()}};
             },
             [this](const temp::Label &label) {
               return Operation{"MOVE #`l0, +(`s0)",
                                {},
                                {m_callingConvention.stackPointer()},
                                {label}};
             },
             [](const auto & /*default*/) {
               assert(false && "Unsupported arg type");
               return Operation{};
             });
         });
}

} // namespace m68k
  // m68k
} // namespace assembly
  // m68k
} // namespace tiger
  // assembly
