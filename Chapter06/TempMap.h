#pragma once
#include <boost/optional/optional_fwd.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace tiger {

// a temporary is just a number
using Temporary = int;

// a label is just a string
using Label = std::string;

// A Map is just a table whose keys are Temp_temps and whose bindings
// are strings. However, one mapping can be layered over another; if σ3 =
// layer(σ1, σ2), this means that look(σ3, t) will first try look(σ1, t), and if
// that fails it will try look(σ2, t).Also, enter(σ3, t, a) will have the effect
// of entering t->a into σ2.
class TempMap {
public:
  void layerOver(TempMap &&under);
  Temporary newTemp();
  boost::optional<std::string> lookup(const Temporary &t);
  friend std::ostream &operator<<(std::ostream &ost, const TempMap &map);

  Label newLabel();

private:
  std::unordered_map<Temporary, std::string> m_map;
  std::unique_ptr<TempMap> m_under;
};

} // namespace tiger
