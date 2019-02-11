#include "x64Machine.h"
#include "Tree.h"
#include "x64Registers.h"

namespace tiger {
namespace x64 {

frame::CallingConvention &Machine::callingConvention() {
  return m_callingConvention;
}

const frame::CallingConvention &Machine::callingConvention() const {
  return m_callingConvention;
}

assembly::CodeGenerator &Machine::codeGenerator() { return m_codeGenerator; }

const assembly::CodeGenerator &Machine::codeGenerator() const {
  return m_codeGenerator;
}

temp::PredefinedRegisters Machine::predefinedRegisters() const {
  using namespace tiger::frame::x64;
  return {{reg(Registers::RAX), "RAX"},     {reg(Registers::RDX), "RDX"},
          {reg(Registers::RCX), "RCX"},     {reg(Registers::RBX), "RBX"},
          {reg(Registers::RSI), "RSI"},     {reg(Registers::RDI), "RDI"},
          {reg(Registers::RBP), "RBP"},     {reg(Registers::RSP), "RSP"},
          {reg(Registers::R8), "R8"},       {reg(Registers::R9), "R9"},
          {reg(Registers::R10), "R10"},     {reg(Registers::R11), "R11"},
          {reg(Registers::R12), "R12"},     {reg(Registers::R13), "R13"},
          {reg(Registers::R14), "R14"},     {reg(Registers::R15), "R15"},
          {reg(Registers::RIP), "RIP"},     {reg(Registers::MM0), "MM0"},
          {reg(Registers::MM1), "MM1"},     {reg(Registers::MM2), "MM2"},
          {reg(Registers::MM3), "MM3"},     {reg(Registers::MM4), "MM4"},
          {reg(Registers::MM5), "MM5"},     {reg(Registers::MM6), "MM6"},
          {reg(Registers::MM7), "MM7"},     {reg(Registers::FP0), "FP0"},
          {reg(Registers::FP1), "FP1"},     {reg(Registers::FP2), "FP2"},
          {reg(Registers::FP3), "FP3"},     {reg(Registers::FP4), "FP4"},
          {reg(Registers::FP5), "FP5"},     {reg(Registers::FP6), "FP6"},
          {reg(Registers::FP7), "FP7"},     {reg(Registers::XMM8), "XMM8"},
          {reg(Registers::XMM9), "XMM9"},   {reg(Registers::XMM10), "XMM10"},
          {reg(Registers::XMM11), "XMM11"}, {reg(Registers::XMM12), "XMM12"},
          {reg(Registers::XMM13), "XMM13"}, {reg(Registers::XMM14), "XMM14"},
          {reg(Registers::XMM15), "XMM15"}, {reg(Registers::XMM16), "XMM16"},
          {reg(Registers::XMM17), "XMM17"}, {reg(Registers::XMM18), "XMM18"},
          {reg(Registers::XMM19), "XMM19"}, {reg(Registers::XMM20), "XMM20"},
          {reg(Registers::XMM21), "XMM21"}, {reg(Registers::XMM22), "XMM22"},
          {reg(Registers::XMM23), "XMM23"}, {reg(Registers::XMM24), "XMM24"},
          {reg(Registers::XMM25), "XMM25"}, {reg(Registers::XMM26), "XMM26"},
          {reg(Registers::XMM27), "XMM27"}, {reg(Registers::XMM28), "XMM28"},
          {reg(Registers::XMM29), "XMM29"}, {reg(Registers::XMM30), "XMM30"},
          {reg(Registers::XMM31), "XMM31"}};
}

} // namespace x64
} // namespace tiger