#include "Program.h"
#include "Compiler.h"
#include "ExpressionParser.h"
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <fstream>

namespace tiger {

namespace spirit = boost::spirit;
namespace qi     = spirit::qi;

namespace detail {

template <typename Iterator>
bool compile(Iterator &first, const Iterator &last) {
  using Grammer      = ExpressionParser<Iterator>;
  using Skipper      = Skipper<Iterator>;
  using ErrorHandler = ErrorHandler<Iterator>;
  using Annotation   = Annotation<Iterator>;

  ErrorHandler errorHandler;
  Annotation annotation;
  Grammer grammer(errorHandler, annotation);
  Skipper skipper;
  Compiler compiler(errorHandler, annotation);

  ast::Expression ast;

  if (qi::phrase_parse(first, last, grammer, skipper, ast) && first == last) {
    simplifyTree(ast);
#ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_OUT << "AST after simplification:\n";
    boost::spirit::traits::print_attribute(BOOST_SPIRIT_DEBUG_OUT, ast);
    BOOST_SPIRIT_DEBUG_OUT << '\n';
#endif

    if (!compiler.compile(ast)) {
      return false;
    }

    return true;
  }

  errorHandler("Parsing failed", "", first);
  return false;
}
} // namespace detail

bool compileFile(const std::string &filename) {
  std::ifstream inputFile(filename, std::ios::in);
  if (!inputFile) {
    std::cerr << "failed to read from " << filename << "\n";
    return false;
  }

  using FileIterator    = std::istreambuf_iterator<char>;
  using ForwardIterator = spirit::multi_pass<FileIterator>;
  using Iterator        = spirit::classic::position_iterator2<ForwardIterator>;

  Iterator first{ForwardIterator(FileIterator(inputFile)), ForwardIterator(),
                 filename};
  Iterator last;

  return detail::compile(first, last);
}

bool compile(const std::string &string) {
  using ForwardIterator = std::string::const_iterator;
  using Iterator        = spirit::classic::position_iterator2<ForwardIterator>;

  Iterator first{ForwardIterator(string.begin()), ForwardIterator(string.end()),
                 "STRING"};
  Iterator last;

  return detail::compile(first, last);
}

} // namespace tiger