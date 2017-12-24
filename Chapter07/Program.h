#pragma once
#include "Tree.h"
#include <string>
#include <boost/optional/optional_fwd.hpp>

namespace tiger 
{

boost::optional<ir::Expression> compileFile(const std::string &filename);

boost::optional<ir::Expression> compile(const std::string &string);

} // namespace tiger