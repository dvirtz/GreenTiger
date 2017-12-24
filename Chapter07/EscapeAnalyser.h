#include "AbstractSyntaxTree.h"
#include <unordered_map>
#include <utility>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>

namespace tiger {
class EscapeAnalyser {
public:
  void analyse(ast::Expression &exp);

private:
  void analyseVar(ast::VarExpression& exp);
  void analyseLet(ast::LetExpression& exp);
  void analyseFuncDec(ast::FunctionDeclarations& decs);
  void analyseVarDec(ast::VarDeclaration& dec);
  void analyseFor(ast::ForExpression& exp);
  template<typename T>
  void analyse(std::vector<T>& v);
  template<typename T>
  void analyse(boost::optional<T>& o);
  template<typename T>
  std::enable_if_t<boost::fusion::traits::is_sequence<T>::value> analyse(T& t);
  template<typename T>
  std::enable_if_t<!boost::fusion::traits::is_sequence<T>::value> analyse(T& t);

  using Environment =
      std::unordered_map<std::string, std::reference_wrapper<bool>>;

  std::vector<Environment> m_environments;
};

template<typename T>
void EscapeAnalyser::analyse(std::vector<T>& v)
{
  std::for_each(v.begin(), v.end(), [this](auto& element) { this->analyse(element);});
}

template<typename T>
std::enable_if_t<boost::fusion::traits::is_sequence<T>::value> EscapeAnalyser::analyse(T& t)
{
  boost::fusion::for_each(t, [this](auto& exp) {this->analyse(exp);});
}

template<typename T>
std::enable_if_t<!boost::fusion::traits::is_sequence<T>::value> EscapeAnalyser::analyse(T& t)
{
}

template<typename T>
void EscapeAnalyser::analyse(boost::optional<T>& o)
{
  if (o) {
    analyse(*o);
  }
}

} // namespace tiger
