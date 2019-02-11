#pragma once
#include "TempRegister.h"
#include <boost/optional/optional_fwd.hpp>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

namespace tiger {

struct CompileResults {
  std::string m_assembly;
  using InterferenceGraph = std::map<std::string, std::set<std::string>>;
  std::vector<InterferenceGraph> m_interferenceGraphs;

  friend std::ostream &operator<<(std::ostream &ost,
                                  const CompileResults &results);
};

using CompileResult = boost::optional<CompileResults>;

CompileResult compileFile(const std::string &arch, const std::string &filename);

CompileResult compile(const std::string &arch, const std::string &string);

} // namespace tiger
