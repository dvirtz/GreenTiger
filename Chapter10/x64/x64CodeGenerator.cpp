#include "x64CodeGenerator.h"
#include "CallingConvention.h"
#include "Tree.h"
#include "x64Registers.h"
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/zip_with.hpp>

namespace tiger {
namespace assembly {
namespace x64 {

using frame::x64::Registers;

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention) :
    assembly::CodeGenerator{
      callingConvention,
      {
        // clang-format off
            Pattern{ir::Move{ir::Call{label()}, callingConvention.returnValue()}, {{InstructionType::OPERATION, "call `l0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>()}}},
            Pattern{ir::Move{imm(), reg()}, {{InstructionType::OPERATION, "mov `d0, `i0", {1, 0}}}},
            Pattern{ir::Move{label(), reg()}, {{InstructionType::OPERATION, "mov `d0, `l0", {1, 0}}}},
            Pattern{ir::Move{imm(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                {{InstructionType::OPERATION, "mov [`s0 + `i0], `i1", {1, 2, 0}}}},
            Pattern{ir::Move{reg(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                {{InstructionType::OPERATION, "mov [`s0 + `i0], `s1", {1, 2, 0}}}},
            Pattern{ir::Move{ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}, reg()}, 
                {{InstructionType::OPERATION, "mov `s0, [`s1 + `i0]", {2, 0, 1}}}},
            Pattern{ir::Jump{label()}, {{InstructionType::JUMP, "jmp `l0", {0}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::MUL, exp(), exp()}, 
                {{InstructionType::MOVE, "mov `d0, `s0", {2, 0}}, {InstructionType::OPERATION, "imul `d0, `s0", {2, 1}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::PLUS, exp(), exp()}, 
                {{InstructionType::MOVE, "mov `d0, `s0", {2, 0}}, {InstructionType::OPERATION, "add `d0, `s0", {2, 1}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::MINUS, exp(), exp()}, 
                {{InstructionType::MOVE, "mov `d0, `s0", {2, 0}}, {InstructionType::OPERATION, "sub `d0, `s0", {2, 1}, {2}}}},
            Pattern{ir::BinaryOperation{ir::BinOp::DIV, exp(), exp()}, 
                {{InstructionType::MOVE, "mov `d0, `s0", {reg(Registers::RAX), 0}}, 
                {InstructionType::OPERATION, "idiv `s0", {1}, {reg(Registers::RAX)}, {reg(Registers::RAX)}}, 
                {InstructionType::MOVE, "mov `d0, `s0", {2, reg(Registers::RAX)}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::EQ, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "je `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::NE, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "jne `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::LT, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "jl `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::GT, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "jg `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::LE, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "jle `l0", {2, 3}}}},
            Pattern{ir::ConditionalJump{ir::RelOp::GE, exp(), exp(), label(), label()},
                {{InstructionType::OPERATION, "sub `d0, `s0", {1, 0}, {1}}, {InstructionType::JUMP, "jge `l0", {2, 3}}}},
            Pattern{ir::Move{reg(), reg()}, {{InstructionType::MOVE, "mov `d0, `s0", {1, 0}}}},
            Pattern{ir::Move{exp(), reg()}, {{InstructionType::MOVE, "mov `d0, `s0", {1, 0}}}},
            Pattern{ir::Move{imm(), exp()}, {{InstructionType::OPERATION, "mov `d0, `i0", {1, 0}}}},
            Pattern{ir::Move{label(), exp()}, {{InstructionType::OPERATION, "mov `d0, `l0", {1, 0}}}},
            Pattern{ir::Move{exp(), ir::MemoryAccess{exp()}}, {{InstructionType::OPERATION, "mov [`s0], `s1", {1, 0}}}},
            Pattern{ir::Move{exp(), exp()}, {{InstructionType::MOVE, "mov `d0, `s0", {1, 0}}}},
            Pattern{ir::MemoryAccess{exp()}, {{InstructionType::OPERATION, "mov `d0, [`s0]", {1, 0}}}},
            Pattern{ir::Expression{imm()}, {{InstructionType::OPERATION, "mov `d0, `i0", {1, 0}}}},
            Pattern{ir::Call{label()}, {{InstructionType::OPERATION, "call `l0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>()}}}, 
            Pattern{ir::Call{exp()}, {{InstructionType::OPERATION, "call `s0", {0}, {}, 
                    callingConvention.callDefinedRegisters() | ranges::to<Arguments>()}}},
            Pattern{ir::Statement{label()}, {{InstructionType::LABEL, "`l0:", {0}}}}
        // clang-format on
      } // namespace assembly
    }   // namespace tiger
{}

Instructions CodeGenerator::translateString(const temp::Label &label,
                                            const std::string &string,
                                            temp::Map & /* tempMap */) {
  return {Label{"`l0:", label}, Operation{{".string " + escape(string)}}};
}

Instructions
  CodeGenerator::translateArgs(const std::vector<ir::Expression> &args,
                               const temp::Map & /*tempMap*/) const {
  namespace rv                 = ranges::views;
  auto const registerArguments = rv::zip_with(
    [](temp::Register argumentRegister, const ir::Expression &arg) {
      return helpers::match(arg)(
        [&](int i) -> Instruction {
          return Operation{"mov `d0, `i0", {argumentRegister}, {}, {}, {i}};
        },
        [&](const temp::Register &reg) -> Instruction {
          return Move{"mov `d0, `s0", {argumentRegister}, {reg}};
        },
        [&](const temp::Label &label) -> Instruction {
          return Operation{"mov `d0, `l0", {argumentRegister}, {}, {label}};
        },
        [](const auto & /*default*/) {
          assert(false && "Unsupported arg type");
          return Instruction{};
        });
    },
    m_callingConvention.argumentRegisters(), args);
  auto const argumentRegisterCount =
    m_callingConvention.argumentRegisters().size();
  if (argumentRegisterCount < args.size()) {
    auto const frameArguments = rv::zip_with(
      [this](size_t index, const ir::Expression &arg) {
        auto const stackPointer = m_callingConvention.stackPointer();
        auto const frameOffset =
          static_cast<int>(index * m_callingConvention.wordSize());
        return helpers::match(arg)(
          [&](int i) -> Instruction {
            return Operation{
              "mov [`s0 + `i0], `i1", {stackPointer}, {}, {}, {frameOffset, i}};
          },
          [&](const temp::Register &reg) -> Instruction {
            return Operation{"mov [`s0 + `i0], `s1",
                             {stackPointer, reg},
                             {},
                             {},
                             {frameOffset}};
          },
          [&](const temp::Label &label) -> Instruction {
            return Operation{"mov [`s0 + `i0], `l0",
                             {stackPointer},
                             {},
                             {label},
                             {frameOffset}};
          },
          [](const auto & /*default*/) {
            assert(false && "Unsupported arg type");
            return Instruction{};
          });
      },
      rv::iota(argumentRegisterCount), args | rv::drop(argumentRegisterCount));
    return rv::concat(registerArguments, frameArguments);
  }

  return registerArguments;
}

} // namespace x64
} // namespace assembly
} // namespace tiger
