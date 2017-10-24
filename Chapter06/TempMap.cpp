/*
 * temp.cpp - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include "TempMap.h"
#include <ostream>
#include <boost/optional.hpp>

namespace tiger {

std::ostream &operator<<(std::ostream &ost, const TempMap &map) {
  for (const auto &it : map.m_map) {
    ost << "t" << it.first << " -> " << it.second << '\n';
  }
  if (map.m_under) {
    ost << "---------\n";
    ost << *map.m_under;
  }

  return ost;
}

void TempMap::layerOver(TempMap &&under) {
  if (m_under) {
    m_under->layerOver(std::move(under));
  } else {
    m_under.reset(&under);
  }
}

Temporary TempMap::newTemp() {
  static const auto MIN_TEMP = 100;
  static auto temps = MIN_TEMP;
  auto res = temps++;
  m_map[res] = std::to_string(res);
  return res;
}

boost::optional<std::string> TempMap::lookup(const Temporary &t) {
  auto it = m_map.find(t);
  if (it != m_map.end())
    return it->second;
  if (m_under)
    return m_under->lookup(t);
  return {};
}

Label TempMap::newLabel() {
  static const auto MIN_LABEL = 0;
  static auto labels = MIN_LABEL;
  return "L" + std::to_string(labels++);
}

} // namespace tiger
