#include "x64CodeGenerator.h"
#include "CallingConvention.h"
#include "Tree.h"
#include <range/v3/view/iota.hpp>
#include <range/v3/view/zip_with.hpp>

namespace tiger {
namespace assembly {
namespace x64 {

CodeGenerator::CodeGenerator(frame::CallingConvention &callingConvention) :
    assembly::CodeGenerator{
      callingConvention,
      {
        // clang-format off
            Pattern{ir::Move{callingConvention.returnValue(), ir::Call{label()}}, Operation{"call `l0"}},
            Pattern{ir::Move{reg(), imm()}, Move{"mov `s0, `i0"}},
            Pattern{ir::Move{reg(), label()}, Move{"mov `s0, `l0"}},
            Pattern{ir::Move{ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}, imm()}, 
                Move{"mov [`s0 + `i0], `i1"}},
            Pattern{ir::Move{reg(), ir::MemoryAccess{ir::BinaryOperation{ir::BinOp::PLUS, reg(), imm()}}}, 
                Move{"mov `s0, [`s1 + `i0]"}},
            Pattern{ir::Jump{label()}, Operation{"jmp `l0"}},
            Pattern{ir::BinaryOperation{ir::BinOp::MUL, exp(), exp()}, 
                Operation{"mov `d0, `s0", "imul `d0, `s1"}},
            Pattern{ir::BinaryOperation{ir::BinOp::PLUS, exp(), exp()}, 
                Operation{"mov `d0, `s0", "add `d0, `s1"}},
            Pattern{ir::BinaryOperation{ir::BinOp::MINUS, exp(), exp()}, 
                Operation{"mov `d0, `s0", "sub `d0, `s1"}},
            Pattern{ir::BinaryOperation{ir::BinOp::DIV, exp(), exp()}, 
                Operation{"mov EAX, `s0", "idiv `s1", "mov `d0, EAX"}},
            Pattern{ir::ConditionalJump{ir::RelOp::EQ, exp(), exp()},
                Operation{"sub `s1, `s0", "je `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::NE, exp(), exp()},
                Operation{"sub `s1, `s0", "jne `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::LT, exp(), exp()},
                Operation{"sub `s1, `s0", "jl `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::GT, exp(), exp()},
                Operation{"sub `s1, `s0", "jg `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::LE, exp(), exp()},
                Operation{"sub `s1, `s0", "jle `l0"}},
            Pattern{ir::ConditionalJump{ir::RelOp::GE, exp(), exp()},
                Operation{"sub `s1, `s0", "jge `l0"}},
            Pattern{ir::Move{reg(), reg()}, Move{"mov `s0, `s1"}},
            Pattern{ir::Move{reg(), exp()}, Move{"mov `s0, `s1"}},
            Pattern{ir::Move{exp(), imm()}, Move{"mov `s0, `i0"}},
            Pattern{ir::Move{exp(), label()}, Move{"mov `s0, `l0"}},
            Pattern{ir::Move{ir::MemoryAccess{reg()}, reg()}, Move{"mov [`s0], `s1"}},
            Pattern{ir::Move{exp(), exp()}, Move{"mov `s0, `s1"}},
            Pattern{ir::MemoryAccess{exp()}, Move{"mov `d0, [`s0]"}},
            Pattern{ir::Expression{imm()}, Move{"mov `d0, `i0"}},
            Pattern{ir::Call{label()}, Operation{"call `l0"}}, 
            Pattern{ir::Call{exp()}, Operation{"call `s0"}},
            Pattern{ir::Statement{label()}, Label{"`l0:"}}
        // clang-format on
      }} {}

Instructions CodeGenerator::translateString(const temp::Label &label,
                                            const std::string &string,
                                            temp::Map & /* tempMap */) {
  return {Label{"`l0:", label}, Operation{{".string " + escape(string)}}};
}

Instructions
  CodeGenerator::translateArgs(const std::vector<ir::Expression> &args,
                               const temp::Map &tempMap) const {
  auto argumentRegisters = m_callingConvention.argumentRegisters();
  auto translateArg      = [&](int index, const std::string &arg) {
    return static_cast<size_t>(index) < argumentRegisters.size()
             ? "mov " + *tempMap.lookup(argumentRegisters[index]) + ", " + arg
             : "push " + arg;
  };
  return ranges::views::zip_with(
    [&](int index, const ir::Expression &arg) {
      return helpers::match(arg)(
        [&](int i) {
          return Move{translateArg(index, "`i0"), {}, {}, {}, {i}};
        },
        [&](const temp::Register &reg) {
          return Move{translateArg(index, "`s0"), {}, {reg}};
        },
        [&](const temp::Label &label) {
          return Move{translateArg(index, "`l0"), {}, {}, {label}};
        },
        [](const auto & /*default*/) {
          assert(false && "Unsupported arg type");
          return Move{};
        });
    },
    ranges::views::ints, args);
}

} // namespace x64
} // namespace assembly
} // namespace tiger
