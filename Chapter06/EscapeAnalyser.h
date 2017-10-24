#include "AbstractSyntaxTree.h"
#include <unordered_map>
#include <utility>

namespace tiger {
class EscapeAnalyser {
public:
  using result_type = bool;
  
  result_type analyse(ast::Expression &exp);

private:
  result_type analyseVar(ast::VarExpression& exp);
  result_type analyseLet(ast::LetExpression& exp);
  result_type analyseFuncDec(ast::FunctionDeclarations& decs);
  result_type analyseVarDec(ast::VarDeclaration& dec);

  using Environment =
      std::unordered_map<std::string, std::reference_wrapper<bool>>;

  std::vector<Environment> m_environments;
};
} // namespace tiger
