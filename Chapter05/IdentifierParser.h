#pragma once
#include "ErrorHandler.h"
#include "Skipper.h"
#include "Annotation.h"
#include "warning_suppress.h"
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()
#include <boost/spirit/include/phoenix_bind.hpp>

namespace tiger {
template <typename Iterator>
class IdentifierParser
  : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, ast::Identifier()> {
public:
  IdentifierParser(ErrorHandler<Iterator> &errorHandler, Annotation<Iterator>& annotation)
    : IdentifierParser::base_type(identifier) {
    namespace spirit = boost::spirit;
    namespace qi = spirit::qi;
    namespace ascii = spirit::ascii;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;

    using ascii::alnum;
    using ascii::alpha;

    using qi::lexeme;
    using qi::raw;
    using qi::on_error;
    using qi::fail;
    using qi::on_success;
    using qi::attr;

    using namespace qi::labels;

    keywords.add
    ("array")
      ("if")
      ("then")
      ("else")
      ("while")
      ("for")
      ("to")
      ("do")
      ("let")
      ("in")
      ("end")
      ("of")
      ("break")
      ("nil")
      ("function")
      ("var")
      ("type")
      ;

    name =
      !lexeme[keywords >> !(alnum | '_')]
      >> raw[lexeme[alpha >> *(alnum | '_')]]
      ;

    identifier = name > attr(42);

    on_error<fail>(identifier,
      phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    ANNOTATE_NODES(
      (identifier)
    )

    BOOST_SPIRIT_DEBUG_NODES(
      (identifier)
    )
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;
  template <typename... Types>
  using symbols = boost::spirit::qi::symbols<Types...>;

  rule<tiger::ast::Identifier()> identifier;
  rule<std::string()> name;

  symbols<char> keywords;
};
} // namespace tiger