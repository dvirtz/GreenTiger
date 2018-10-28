/*
 * temp.cpp - functions to create and manipulate Register variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include "TempMap.h"
#include "warning_suppress.h"
#include <boost/optional.hpp>
#include <ostream>

namespace tiger {

namespace temp {

std::ostream &operator<<(std::ostream &ost, const Map &map) {
  for (const auto &it : map.m_map) {
    ost << "t" << it.first << " -> " << it.second << '\n';
  }
  if (map.m_under) {
    ost << "---------\n";
    ost << *map.m_under;
  }

  return ost;
}

Map::Map(const PredefinedRegisters &predefinedRegisters /*= {}*/) :
    m_map{predefinedRegisters} {}

void Map::layerOver(Map &&under) {
  if (m_under) {
    m_under->layerOver(std::move(under));
  } else {
    m_under.reset(&under);
  }
}

Register Map::newTemp() {
  Register res{m_nextTemp};
  m_map[res] = "t" + std::to_string(m_nextTemp++);
  return res;
}

boost::optional<std::string> Map::lookup(const Register &t) const {
  auto it = m_map.find(t);
  if (it != m_map.end())
    return it->second;
  if (m_under)
    return m_under->lookup(t);
  return {};
}

Label Map::newLabel() {
  static const auto MIN_LABEL = 0;
  static auto labels          = MIN_LABEL;
  return Label{"L" + std::to_string(labels++)};
}

Label Map::namedLabel(const std::string &name) { return Label{name}; }

} // namespace temp
} // namespace tiger
