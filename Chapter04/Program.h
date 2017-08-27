#pragma once
#include <string>
#include "AbstractSyntaxTree.h"

namespace tiger {

bool parseFile(const std::string &filename, ast::Expression& ast);

bool parse(const std::string &string, ast::Expression& ast);

} // namespace tiger