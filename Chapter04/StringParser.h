#pragma once
#include "ErrorHandler.h"
#include "Annotation.h"
#include "Skipper.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

namespace tiger {
template <typename Iterator>
class StringParser
  : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, ast::StringExpression()> {
public:
  StringParser(ErrorHandler<Iterator> &errorHandler, Annotation<Iterator>& annotation)
    : StringParser::base_type(string) {
    namespace spirit = boost::spirit;
    namespace qi = spirit::qi;
    namespace ascii = spirit::ascii;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;

    using qi::lit;
    using qi::on_error;
    using qi::fail;
    using qi::on_success;
    using qi::uint_parser;
    using qi::skip;

    using ascii::char_;
    using ascii::space;

    using phoenix::bind;

    using namespace qi::labels;

    string
      = lit('"')
      > skip(skipper.alias())[*(escapeCharacter | (char_ - '"'))]
      > '"';

    escapeCharacter
      %= lit('\\')
      > (lit('n')[_val = '\n']                                                  // newline
        | lit('t')[_val = '\t']                                                 // tab
        | (lit('^') > uint_parser<char>()[_pass = phoenix::bind(iscntrl, _1)])  // control character
        | uint_parser<char, 10, 1, 3>()                                         // ascii code
        | char_('"')                                                            // double quote
        | char_('\\')                                                           // backslash
        )
      ;

    skipper = '\\' >> +space > '\\';

    on_error<fail>(string,
      phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

//     on_success(string,
//       phoenix::bind(annotation, _val, _1));

    BOOST_SPIRIT_DEBUG_NODES(
      (string)
      (escapeCharacter)
    )
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  rule<ast::StringExpression()> string;
  // no skipping
  boost::spirit::qi::rule<Iterator, char()> escapeCharacter;
  boost::spirit::qi::rule<Iterator> skipper;
};
} // namespace tiger