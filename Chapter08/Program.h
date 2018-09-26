#pragma once
#include "Fragment.h"
#include "Tree.h"
#include <boost/optional/optional_fwd.hpp>
#include <string>

namespace tiger {

using CompileResult = boost::optional<ir::Statements>;

CompileResult compileFile(const std::string &filename);

CompileResult compile(const std::string &string);

std::ostream &
  operator<<(std::ostream &ost,
             std::pair<ir::Expression, FragmentList> const &compileResult);

} // namespace tiger
