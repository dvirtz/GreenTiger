#include "FlowGraph.h"
#include "variantMatch.h"
#include "warning_suppress.h"
#include <boost/graph/graph_utility.hpp>
MSC_DIAG_OFF(4459)
#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/to_container.hpp>
#include <range/v3/view.hpp>
MSC_DIAG_ON()
#include <unordered_map>

namespace tiger {
namespace regalloc {
FlowGraph::FlowGraph(const assembly::Instructions &instructions) :
    base{instructions.size()}, m_uses{instructions.size()},
    m_defs{instructions.size()}, m_isMove{instructions.size()} {
  using namespace assembly;
  namespace rv = ranges::views;
  namespace ra = ranges::actions;

  const std::unordered_map<temp::Label, int> label_map =
    rv::zip(instructions, rv::ints)
    | rv::filter([](const std::pair<const Instruction &, int> &p) {
        return helpers::hasType<Label>(p.first);
      })
    | rv::transform([](const std::pair<const Instruction &, int> &p) {
        return std::make_pair(boost::get<Label>(p.first).m_label, p.second);
      });

  // using namespace ranges::function_objects;

  auto const add_edge = [this](size_t from, size_t to) {
    if (to < boost::num_vertices(*this)) {
      boost::add_edge(from, to, *this);
    }
  };

  ranges::for_each(rv::zip(rv::ints, instructions), [&](const auto &p) {
    auto const processInstruction = [&](int index,
                                        const Instruction &instruction) {
      helpers::match(instruction)(
        [&](const Jump &jump) {
          ranges::for_each(jump.m_labels, [&](const temp::Label &label) {
            auto it = label_map.find(label);
            if (it != label_map.end()) {
              add_edge(index, it->second);
            }
          });
        },
        [&](const auto & /*default*/) { add_edge(index, index + 1); });
      m_uses[index]   = instruction.sources() | ra::sort | ra::unique;
      m_defs[index]   = instruction.destinations() | ra::sort | ra::unique;
      m_isMove[index] = instruction.isMove();
      (*this)[index]  = instruction;
    };
    processInstruction(p.first, p.second);
  });

  //boost::print_graph(*this);
}

const temp::Registers &FlowGraph::uses(vertex_descriptor v) const {
  return m_uses[v];
}
const temp::Registers &FlowGraph::defs(vertex_descriptor v) const {
  return m_defs[v];
}
bool FlowGraph::isMove(vertex_descriptor v) const { return m_isMove[v]; }

} // namespace regalloc
} // namespace tiger