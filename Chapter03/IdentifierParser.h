#pragma once
#include "ErrorHandler.h"
#include "Skipper.h"
#include "warning_suppress.h"
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()

namespace tiger {
template <typename Iterator>
class IdentifierParser
    : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>> {
public:
  IdentifierParser(ErrorHandler<Iterator> & /* errorHandler */) :
      IdentifierParser::base_type(identifier) {
    namespace spirit = boost::spirit;
    namespace qi     = spirit::qi;
    namespace ascii  = spirit::ascii;

    using ascii::alnum;
    using ascii::alpha;

    using qi::lexeme;
    using qi::raw;

    keywords.add("array")("if")("then")("else")("while")("for")("to")("do")(
      "let")("in")("end")("of")("break")("nil")("function")("var")("type");

    identifier =
      !lexeme[keywords >> !(alnum | '_')] >> lexeme[alpha >> *(alnum | '_')];

    BOOST_SPIRIT_DEBUG_NODES((identifier))
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;
  template <typename... Types>
  using symbols = boost::spirit::qi::symbols<Types...>;

  rule<> identifier;

  symbols<char> keywords;
};
} // namespace tiger