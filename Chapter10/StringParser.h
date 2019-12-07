#pragma once
#include "Annotation.h"
#include "ErrorHandler.h"
#include "Skipper.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

namespace tiger {
template <typename Iterator>
class StringParser
    : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>,
                                        ast::StringExpression()> {
public:
  StringParser(ErrorHandler<Iterator> &errorHandler,
               Annotation<Iterator> &annotation) :
      StringParser::base_type(string) {
    namespace spirit  = boost::spirit;
    namespace qi      = spirit::qi;
    namespace ascii   = spirit::ascii;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;
    using namespace boost::spirit::labels;

    using qi::fail;
    using qi::lit;
    using qi::on_error;
    using qi::skip;
    using qi::uint_parser;

    using ascii::char_;
    using ascii::space;

    using phoenix::bind;

    string = lit('"')
             > skip(skipper.alias())[*(escapeCharacter | controlCharacter
                                       | asciiCode | (char_ - '"'))]
             > '"';

    escapeCharacter.add("\\n", '\n')("\\t", '\t')("\\\"", '\"')("\\\\", '\\');

    controlCharacter %=
      lit("\\^") > uint_parser<char>(); //[_pass = phoenix::bind(iscntrl, _1)];

    asciiCode = lit("\\") >> uint_parser<char, 10, 1, 3>();

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"
#endif
    skipper = lit("\\") >> +space > '\\';
#ifdef __clang__
#pragma clang diagnostic pop
#endif

    on_error<fail>(string,
                   phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    ANNOTATE_NODES((string))

    BOOST_SPIRIT_DEBUG_NODES((string)(controlCharacter)(asciiCode))
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  rule<ast::StringExpression()> string;
  // no skipping
  // boost::spirit::qi::rule<Iterator, char()> escapeCharacter;
  boost::spirit::qi::rule<Iterator> skipper;
  boost::spirit::qi::rule<Iterator, char()> controlCharacter;
  boost::spirit::qi::rule<Iterator, char()> asciiCode;

  boost::spirit::qi::symbols<const char, char> escapeCharacter;
};
} // namespace tiger
