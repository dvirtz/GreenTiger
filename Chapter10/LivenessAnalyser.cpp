#include "LivenessAnalyser.h"
#include "Assembly.h"
#include "FlowGraph.h"
#include "irange.h"
#include "variantMatch.h"
#include "warning_suppress.h"
#include <boost/graph/graph_utility.hpp>
MSC_DIAG_OFF(4239 4459 4913)
#include <range/v3/action/join.hpp>
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/algorithm/for_each.hpp>
#ifdef _MSC_VER
#include <range/v3/algorithm/set_algorithm.hpp>
#else
#include <range/v3/view/set_algorithm.hpp>
#endif
#include <range/v3/algorithm/is_sorted.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
MSC_DIAG_ON()

namespace rv = ranges::view;
namespace ra = ranges::action;

namespace tiger {
namespace regalloc {

LivenessAnalyser::LivenessAnalyser(const FlowGraph &flowGraph,
                                   const temp::Map &tempMap) {
  calculateLiveness(flowGraph);
  generateGraph(flowGraph, tempMap);
}

void LivenessAnalyser::generateGraph(const FlowGraph &flowGraph,
                                     const temp::Map &/*tempMap*/) {

  ranges::for_each(
    irange(boost::vertices(flowGraph)),
    [&](const FlowGraph::vertex_descriptor &v) {
      const auto add_edge = [&](const temp::Register &a,
                                const temp::Register &b) {
        if (a != b) {
          boost::add_edge(boost::add_vertex(a, m_interferenceGraph),
                          boost::add_vertex(b, m_interferenceGraph),
                          m_interferenceGraph);
        }
      };
      if (flowGraph.isMove(v)) {
        assert(flowGraph.uses(v).size() == 1
               && "Move should only have one use");
        assert(flowGraph.defs(v).size() == 1
               && "Move should only have one def");
        const auto def = flowGraph.defs(v).front();
        const auto use = flowGraph.uses(v).front();
        ranges::for_each(
          m_liveOuts[v]
            | rv::filter([&](const temp::Register &t) { return t != use; }),
          [&](const temp::Register &t) { add_edge(def, t); });
        m_copiedRegisters.emplace_back(use, def);
      } else {
        ranges::for_each(flowGraph.defs(v), [&](const temp::Register &def) {
          ranges::for_each(m_liveOuts[v],
                           [&](const temp::Register &t) { add_edge(def, t); });
        });
      }
    });

  // ranges::for_each(
  //  irange(boost::vertices(m_interferenceGraph)),
  //  [&tempMap, this](const InterferenceGraph::vertex_descriptor &v) {
  //    std::cout << *tempMap.lookup(m_interferenceGraph[v]) << " <--> ";
  //    ranges::for_each(
  //      irange(boost::adjacent_vertices(v, m_interferenceGraph)),
  //      [&tempMap, this](const InterferenceGraph::vertex_descriptor &u) {
  //        std::cout << *tempMap.lookup(m_interferenceGraph[u]) << ' ';
  //      });
  //    std::cout << '\n';
  //  });
}

void LivenessAnalyser::calculateLiveness(const FlowGraph &flowGraph) {
  auto verticesCount = boost::num_vertices(flowGraph);
  m_liveIns.resize(verticesCount);
  m_liveOuts.resize(verticesCount);
  LiveRegisters prevLiveIns(verticesCount), prevLiveOuts(verticesCount);
  do {
    ranges::for_each(irange(boost::vertices(flowGraph)), [&](const auto &v) {
      prevLiveIns[v]  = std::move(m_liveIns[v]);
      prevLiveOuts[v] = std::move(m_liveOuts[v]);
      // in[n] is all the variables in use[n], plus all the variables in
      // out[n] and not in def[n]
      auto use = flowGraph.uses(v);
      auto def = flowGraph.defs(v);
      assert(ranges::is_sorted(use) && "registers must be sorted");
      assert(ranges::is_sorted(def) && "registers must be sorted");
#ifdef _MSC_VER
      temp::Registers temp;
      temp.reserve(prevLiveOuts[v].size());
      ranges::set_difference(prevLiveOuts[v], def, ranges::back_inserter(temp));
      m_liveIns[v].reserve(use.size() + temp.size());
      ranges::set_union(use, temp, ranges::back_inserter(m_liveIns[v]));
#else
      m_liveIns[v] = rv::set_union(use, rv::set_difference(prevLiveOuts[v], def));
#endif
      // out[n] is the union of the live-in sets of all successors of n
      m_liveOuts[v] =
        irange(boost::adjacent_vertices(v, flowGraph))
        | rv::transform(
            [&](const FlowGraph::vertex_descriptor &v) { return m_liveIns[v]; })
        | ra::join | ra::sort | ra::unique;
    });

  } while (m_liveIns != prevLiveIns || m_liveOuts != prevLiveOuts);
}

} // namespace regalloc
} // namespace tiger