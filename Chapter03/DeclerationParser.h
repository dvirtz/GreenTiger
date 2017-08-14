#pragma once
#include "ErrorHandler.h"
#include "Skipper.h"
#include <boost/spirit/include/qi.hpp>

namespace tiger {
template <typename Iterator>
class DeclerationParser
  : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>> {
public:
  DeclerationParser(base_type& expressionParser, base_type& identifierParser, ErrorHandler<Iterator> &errorHandler)
    : base_type(declaration), expression(expressionParser), identifier(identifierParser) {
    namespace spirit = boost::spirit;
    namespace qi = spirit::qi;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;
    using namespace qi::labels;

    using qi::lit;
    using qi::on_error;
    using qi::fail;

    typeFields = -((identifier > ':' > identifier) % ",");

    typeValue
      = identifier
      | (lit('{') > typeFields > '}')
      | (lit("array") > "of" > identifier)
      ;

    typeDeclaration
      = lit("type")
      > identifier
      > "="
      > typeValue
      ;

    variableDeclaration
      = lit("var")
      > identifier
      > -(lit(':') >> identifier)
      > ":="
      > expression
      ;

    functionDeclaration 
      = lit("function") 
      > identifier 
      > '(' 
      > typeFields 
      > ')' 
      > -(lit(':') > identifier) 
      > '=' 
      > expression
      ;

    declaration
      = typeDeclaration
      | variableDeclaration
      | functionDeclaration
      ;

    on_error<fail>(declaration,
      phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    BOOST_SPIRIT_DEBUG_NODES(
      (typeDeclaration)
      (typeValue)
      (typeFields)
      (variableDeclaration)
      (functionDeclaration)
      (declaration)
    )
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  base_type& expression;
  base_type& identifier;

  rule<> typeDeclaration;
  rule<> typeValue;
  rule<> typeFields;
  rule<> variableDeclaration;
  rule<> functionDeclaration;
  rule<> declaration;
};
} // namespace tiger