#pragma once
#include "strong_typedef.h"
#include <boost/optional/optional_fwd.hpp>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>

namespace tiger {

namespace temp {

// a register is just a number
using Register = helpers::strong_typedef<int>;

// a label is just a string
using Label = helpers::strong_typedef<std::string>;

constexpr auto MIN_TEMP = 100;

// A Map is just a table whose keys are Temp_temps and whose bindings
// are strings. However, one mapping can be layered over another; if σ3 =
// layer(σ1, σ2), this means that look(σ3, t) will first try look(σ1, t), and if
// that fails it will try look(σ2, t).Also, enter(σ3, t, a) will have the effect
// of entering t->a into σ2.
class Map {
public:
  void layerOver(Map &&under);
  Register newTemp();
  boost::optional<std::string> lookup(const Register &t);
  friend std::ostream &operator<<(std::ostream &ost, const Map &map);

  Label newLabel();
  Label namedLabel(const std::string &name);

private:
  std::unordered_map<Register, std::string> m_map;
  std::unique_ptr<Map> m_under;
  int m_nextTemp = MIN_TEMP;
};

} // namespace temp
} // namespace tiger
