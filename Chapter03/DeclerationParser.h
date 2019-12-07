#pragma once
#include "ErrorHandler.h"
#include "Skipper.h"
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>

namespace tiger {
template <typename Iterator> class ExpressionParser;

template <typename Iterator> class IdentifierParser;

template <typename Iterator>
class DeclerationParser
    : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>> {
public:
  DeclerationParser(ExpressionParser<Iterator> &expressionParser,
                    IdentifierParser<Iterator> &identifierParser,
                    ErrorHandler<Iterator> &errorHandler) :
      DeclerationParser::base_type(declaration),
      expression(expressionParser), identifier(identifierParser) {
    namespace spirit  = boost::spirit;
    namespace qi      = spirit::qi;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;
    using namespace boost::spirit::labels;

    using qi::fail;
    using qi::lit;
    using qi::on_error;

    typeFields = -((identifier > ':' > identifier) % ",");

    typeValue = identifier | (lit('{') > typeFields > '}')
                | (lit("array") > "of" > identifier);

    typeDeclaration = lit("type") > identifier > "=" > typeValue;

    variableDeclaration =
      lit("var") > identifier > -(lit(':') >> identifier) > ":=" > expression;

    functionDeclaration = lit("function") > identifier > '(' > typeFields > ')'
                          > -(lit(':') > identifier) > '=' > expression;

    declaration = typeDeclaration | variableDeclaration | functionDeclaration;

    on_error<fail>(declaration,
                   phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    BOOST_SPIRIT_DEBUG_NODES((typeDeclaration)(typeValue)(typeFields)(
      variableDeclaration)(functionDeclaration)(declaration))
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  ExpressionParser<Iterator> &expression;
  IdentifierParser<Iterator> &identifier;

  rule<> typeDeclaration;
  rule<> typeValue;
  rule<> typeFields;
  rule<> variableDeclaration;
  rule<> functionDeclaration;
  rule<> declaration;
};
} // namespace tiger