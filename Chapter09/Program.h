#pragma once
#include "Assembly.h"
#include "Fragment.h"
#include <boost/optional/optional_fwd.hpp>
#include <string>

namespace tiger {

using CompileResult = boost::optional<std::string>;

CompileResult compileFile(const std::string &arch, const std::string &filename);

CompileResult compile(const std::string &arch, const std::string &string);

} // namespace tiger
