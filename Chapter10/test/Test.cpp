#include "Test.h"
#include "CallingConvention.h"
#include "FlowGraph.h"
#include "warning_suppress.h"
MSC_DIAG_OFF(4459)
#include "MachineRegistrar.h"
MSC_DIAG_ON()
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/mismatch.hpp>
#include <range/v3/distance.hpp>
#include <range/v3/view/concat.hpp>
#include <range/v3/view/for_each.hpp>
#include <range/v3/view/zip.hpp>

tiger::Machine const &TestFixture::machine() const {
  static auto const machine = tiger::createMachine(arch);
  return *machine;
}

tiger::frame::CallingConvention const &TestFixture::callingConvention() const {
  static auto const &callingConvention = machine().callingConvention();
  return callingConvention;
}

temp::Register TestFixture::returnReg() const {
  static auto const res = callingConvention().returnValue();
  return res;
}

int TestFixture::wordSize() const {
  static auto const res = callingConvention().wordSize();
  return res;
}

temp::Register TestFixture::framePointer() const {
  static auto const res = callingConvention().framePointer();
  return res;
}

temp::Register TestFixture::stackPointer() const {
  static auto const res = callingConvention().stackPointer();
  return res;
}

const temp::Registers &TestFixture::liveAtExitRegisters() const {
  static auto const res = callingConvention().liveAtExitRegisters();
  return res;
}

const temp::PredefinedRegisters &TestFixture::predefinedRegisters() const {
  static auto const res = machine().predefinedRegisters();
  return res;
}

const temp::Registers &TestFixture::argumentRegisters() const {
  static auto const res = callingConvention().argumentRegisters();
  return res;
}

const temp::Registers &TestFixture::callDefinedRegisters() const {
  static auto const res = callingConvention().callDefinedRegisters();
  return res;
}

tiger::CompileResults
  TestFixture::checkedCompile(const std::string &string) const {
  auto res = tiger::compile(arch, string);
  REQUIRE(res);
  return *res;
}

std::string TestFixture::binOp(ir::BinOp op) const {
  if (arch == "m68k") {
    switch (op) {
      case tiger::ir::BinOp::PLUS:
        return "ADD";
      case tiger::ir::BinOp::MUL:
        return "MULS.L";
      case tiger::ir::BinOp::MINUS:
        return "SUB";
      case tiger::ir::BinOp::DIV:
        return "DIVS.L";
      default:
        break;
    }
  } else {
    switch (op) {
      case tiger::ir::BinOp::PLUS:
        return "add";
      case tiger::ir::BinOp::MUL:
        return "imul";
      case tiger::ir::BinOp::MINUS:
        return "sub";
      case tiger::ir::BinOp::DIV:
        return "idiv";
      default:
        break;
    }
  }

  assert(false && "Not implemented");
  return "";
}

std::string TestFixture::conditionalJump(ir::RelOp op) const {
  if (arch == "m68k") {
    std::stringstream sst;
    sst << op;
    return "B" + sst.str();
  } else {
    switch (op) {
      case ir::RelOp::EQ:
        return "je";
      case ir::RelOp::NE:
        return "jne";
      case ir::RelOp::LT:
        return "jl";
      case ir::RelOp::GT:
        return "jg";
      case ir::RelOp::LE:
        return "jle";
      case ir::RelOp::GE:
        return "jge";
      default:
        break;
    }
  }

  assert(false && "Not implemented");
  return "";
}

template <typename R, typename U>
class RangeMatcher : public Catch::MatcherBase<U> {
public:
  RangeMatcher(const R &r) : m_r{r} {}

  bool match(const U &other) const override {
    using namespace ranges;
    using namespace std::string_literals;
    using Catch::Detail::stringify;

    if (distance(other) > distance(m_r)) {
      m_message = "of lengths mistmatch";
      return false;
    }

    auto &&[first, last] = mismatch(other, m_r);
    if (first != end(other)) {
      m_message = "at index "s + stringify(distance(begin(other), first))
                  + ": " + stringify(*first) + " != " + stringify(*last);
      return false;
    }

    return true;
  }

  std::string describe() const override {
    return "not equal\n" + Catch::Detail::stringify(m_r) + "\nbecause\n"
           + m_message;
  }

private:
  const R &m_r;
  mutable std::string m_message;
};

template <typename U, typename R> RangeMatcher<R, U> Equals(const R &r) {
  return RangeMatcher<R, U>{r};
}

void TestFixture::checkInterference(
  const InterferenceGraphs &interefernceGraphs) const {
  REQUIRE(ranges::distance(interefernceGraphs)
          == ranges::distance(m_interferences));
  ranges::for_each(ranges::views::zip(interefernceGraphs, m_interferences),
                   [](const auto &p) {
                     REQUIRE_THAT(p.first, Equals<decltype(p.first)>(p.second));
                   });
}

void TestFixture::addInterferenceImpl(const temp::Register &r,
                                      const temp::Register &s) {
  if (r != s) {
    auto &interferenceGraph = m_interferences.back();
    auto first              = regToString(r);
    auto second             = regToString(s);
    interferenceGraph[first].emplace(second);
    interferenceGraph[second].emplace(first);
  }
}

std::string TestFixture::regToString(const temp::Register &reg) const {
  static const auto predefinedRegisters = this->predefinedRegisters();
  auto const it                         = predefinedRegisters.find(reg);
  if (it != predefinedRegisters.end()) {
    return it->second;
  }

  return "t" + std::to_string(type_safe::get(reg));
}

temp::Register TestFixture::actual(const Reg &reg) {
  return helpers::match(reg)([](const temp::Register &r) { return r; },
                             [](const OptReg &r) { return *r; });
}
