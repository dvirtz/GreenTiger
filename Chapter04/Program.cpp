#include "Program.h"
#include "ExpressionParser.h"
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <fstream>

namespace tiger {

namespace spirit = boost::spirit;
namespace qi     = spirit::qi;

namespace detail {

template <typename Iterator>
bool parse(Iterator &first, const Iterator &last, ast::Expression &ast) {
  using Grammer      = ExpressionParser<Iterator>;
  using Skipper      = Skipper<Iterator>;
  using ErrorHandler = ErrorHandler<Iterator>;
  using Annotation   = Annotation<Iterator>;

  ErrorHandler errorHandler;
  Annotation annotation;
  Grammer grammer(errorHandler, annotation);
  Skipper skipper;

  if (qi::phrase_parse(first, last, grammer, skipper, ast) && first == last) {
    simplifyTree(ast);
#ifdef BOOST_SPIRIT_DEBUG
    BOOST_SPIRIT_DEBUG_OUT << "AST after simplification:\n";
    boost::spirit::traits::print_attribute(BOOST_SPIRIT_DEBUG_OUT, ast);
    BOOST_SPIRIT_DEBUG_OUT << '\n';
#endif
    return true;
  }

  errorHandler("Parsing failed", "", first);
  return false;
}
} // namespace detail

bool parseFile(const std::string &filename, ast::Expression &ast) {
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

  return detail::parse(first, last, ast);
}

bool parse(const std::string &string, ast::Expression &ast) {
  using ForwardIterator = std::string::const_iterator;
  using Iterator        = spirit::classic::position_iterator2<ForwardIterator>;

  Iterator first{ForwardIterator(string.begin()), ForwardIterator(string.end()),
                 "STRING"};
  Iterator last;

  return detail::parse(first, last, ast);
}

} // namespace tiger