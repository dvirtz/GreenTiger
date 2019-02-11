#pragma once
#include "FlowGraph.h"
#include "TempRegister.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/multi_index/global_fun.hpp>

namespace boost {
namespace graph {

inline const int &get_underlying_reg(const tiger::temp::Register &reg) {
  return type_safe::get(reg);
}

/// Use the City name as a key for indexing cities in a graph
template <> struct internal_vertex_name<tiger::temp::Register> {
  typedef multi_index::global_fun<const tiger::temp::Register &, const int &,
                                  &get_underlying_reg>
    type;
};

/// Allow the graph to build cities given only their names (filling in
/// the defaults for fields).
template <> struct internal_vertex_constructor<tiger::temp::Register> {
  typedef vertex_from_name<tiger::temp::Register> type;
};

} // namespace graph
} // namespace boost

namespace tiger {
namespace regalloc {

class InterferenceGraph
    : public boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS,
                                   temp::Register> {
public:
  using base = boost::adjacency_list<boost::setS, boost::vecS,
                                     boost::undirectedS, temp::Register>;
  using base::base;
};

class LivenessAnalyser {
public:
  LivenessAnalyser(const FlowGraph &flowGraph, const temp::Map &tempMap);

  const InterferenceGraph &interferenceGraph() const { return m_interferenceGraph; }

private:
  using CopiedRegisters =
    std::vector<std::pair<temp::Register, temp::Register>>;

  void calculateLiveness(const FlowGraph &flowGraph);
  void generateGraph(const FlowGraph &flowGraph, const temp::Map &tempMap);

  LiveRegisters m_liveIns;
  LiveRegisters m_liveOuts;
  InterferenceGraph m_interferenceGraph;
  CopiedRegisters m_copiedRegisters;
};
} // namespace regalloc
} // namespace tiger