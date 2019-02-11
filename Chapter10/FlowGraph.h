#pragma once
#include "TempRegister.h"
#include "Assembly.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/dynamic_bitset.hpp>

namespace tiger {

namespace regalloc {

using LiveRegisters = std::vector<temp::Registers>;

class FlowGraph
    : public boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                   assembly::Instruction>
{
public:
  FlowGraph(const assembly::Instructions &instructions);

  const temp::Registers &uses(vertex_descriptor v) const;
  const temp::Registers &defs(vertex_descriptor v) const;
  bool isMove(vertex_descriptor v) const;

private:
  using base = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                     assembly::Instruction>;
  LiveRegisters m_uses;
  LiveRegisters m_defs;
  boost::dynamic_bitset<> m_isMove;
};

} // namespace regalloc
} // namespace tiger