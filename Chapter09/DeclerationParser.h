#pragma once
#include "ErrorHandler.h"
#include "Skipper.h"
#include "Annotation.h"

#include "warning_suppress.h"
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>

namespace tiger {
template <typename Iterator>
class DeclerationParser
  : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, tiger::ast::Declaration()> {
public:
  DeclerationParser(boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, tiger::ast::Expression()>& expressionParser,
    boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, tiger::ast::Identifier()>& identifierParser,
    ErrorHandler<Iterator> &errorHandler,
    Annotation<Iterator>& annotation)
    : DeclerationParser::base_type(declaration), expression(expressionParser), identifier(identifierParser) {
    namespace spirit = boost::spirit;
    namespace qi = spirit::qi;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;
    using namespace qi::labels;

    using qi::lit;
    using qi::on_error;
    using qi::fail;
    using qi::on_success;
    using phoenix::push_back;
    using phoenix::at_c;

    typeFields = -((identifier > ':' > identifier) % ",");

    nameType = identifier;
    
    recordType = lit('{') > typeFields > '}';

    arrayType = lit("array") > "of" > identifier;

    typeValue
      = nameType 
      | recordType 
      | arrayType
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
      = +typeDeclaration
      | variableDeclaration
      | +functionDeclaration
      ;

    on_error<fail>(declaration,
      phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    ANNOTATE_NODES(
      (typeDeclaration)
      (variableDeclaration)
      (functionDeclaration)
    )

    BOOST_SPIRIT_DEBUG_NODES(
      (typeDeclaration)
      (typeValue)
      (typeFields)
      (variableDeclaration)
      (functionDeclaration)
      (declaration)
      (nameType)
      (recordType)
      (arrayType)
    )
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, tiger::ast::Expression()>& expression;
  boost::spirit::qi::grammar<Iterator, Skipper<Iterator>, tiger::ast::Identifier()>& identifier;

  rule<ast::TypeDeclaration()>      typeDeclaration;
  rule<ast::Type()>                 typeValue;
  rule<ast::Fields()>               typeFields;
  rule<ast::VarDeclaration()>       variableDeclaration;
  rule<ast::FunctionDeclaration()>  functionDeclaration;
  rule<ast::Declaration()>          declaration;
  rule<ast::NameType()>             nameType;
  rule<ast::RecordType()>           recordType;
  rule<ast::ArrayType()>            arrayType;
};
} // namespace tiger