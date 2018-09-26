#pragma once
#include "DeclerationParser.h"
#include "ErrorHandler.h"
#include "IdentifierParser.h"
#include "Skipper.h"
#include "warning_suppress.h"
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()

namespace tiger {

template <typename Iterator>
class ExpressionParser
    : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>> {
public:
  ExpressionParser(ErrorHandler<Iterator> &errorHandler) :
      ExpressionParser::base_type(expression), identifier(errorHandler),
      declaration(*this, identifier, errorHandler) {
    namespace spirit  = boost::spirit;
    namespace qi      = spirit::qi;
    namespace ascii   = spirit::ascii;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;

    using ascii::char_;
    using ascii::cntrl;
    using ascii::digit;
    using ascii::space;
    using qi::_1;
    using qi::_val;
    using qi::fail;
    using qi::lexeme;
    using qi::lit;
    using qi::on_error;
    using qi::repeat;
    using qi::uint_;

    using namespace qi::labels;

    lvalue =
      identifier >> *((lit('.') > identifier) | (lit('[') > expression > ']'));

    sequence = lit('(') >> -(expression % ';') > ')';

    integer = uint_;

    string = lexeme['"' > *(escapeCharacter | (char_ - '"')) > '"'];

    escapeCharacter = lit('\\') > (lit('n')             // newline
                                   | 't'                // tab
                                   | (lit('^') > cntrl) // control character
                                   | repeat(3)[digit]   // ascii code
                                   | '"'                // double quote
                                   | '\\'               // backslash
                                   | (*space > '\\')    // multi-line
                                  );

    functionCall = identifier >> '(' > -(expression % ',') > ')';

    primaryExpression =
      lit("nil") | string | integer | functionCall | lvalue | sequence;

    unaryExpression = primaryExpression | ('-' > unaryExpression);

    multiplicativeExpression =
      unaryExpression >> *(char_("*/") > unaryExpression);

    additiveExpression =
      multiplicativeExpression >> *(char_("+-") > multiplicativeExpression);

    comparisonExpression =
      additiveExpression
      // comparison operators do not associate
      >> -((lit("=") | "<>" | ">=" | "<=" | ">" | "<") > additiveExpression);

    booleanExpression = booleanAnd | booleanOr;

    booleanAnd =
      comparisonExpression >> '&' > (booleanAnd | comparisonExpression);

    booleanOr =
      comparisonExpression >> '|' > (booleanOr | comparisonExpression);

    record = identifier >> '{' > -((identifier > '=' > expression) % ',') > '}';

    array = identifier >> '[' >> expression >> ']' >> "of" > expression;

    assignment = lvalue >> ":=" > expression;

    ifThenElse =
      lit("if") >> expression >> "then" >> expression >> -("else" > expression);

    whileLoop = lit("while") > expression > "do" > expression;

    forLoop = lit("for") > identifier > ":=" > expression > "to" > expression
              > "do" > expression;

    breakExpression = lit("break");

    let = lit("let") > *declaration > "in" > -(expression % ';') > lit("end");

    expression = record | array | assignment | ifThenElse | whileLoop | forLoop
                 | breakExpression | let | booleanExpression
                 | comparisonExpression;

    on_error<fail>(expression,
                   phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    BOOST_SPIRIT_DEBUG_NODES((expression)(lvalue)(sequence)(integer)(string)(
      functionCall)(additiveExpression)(comparisonExpression)(
      booleanExpression)(record)(array)(assignment)(ifThenElse)(whileLoop)(
      forLoop)(breakExpression)(let)(multiplicativeExpression)(
      primaryExpression)(unaryExpression)(booleanAnd)(booleanOr))
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type>
  using rule = boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature>;

  IdentifierParser<Iterator> identifier;
  DeclerationParser<Iterator> declaration;

  rule<> expression;
  rule<> lvalue;
  rule<> sequence;
  rule<> integer;
  rule<> string;
  rule<> functionCall;
  rule<> additiveExpression;
  rule<> comparisonExpression;
  rule<> booleanExpression;
  rule<> record;
  rule<> array;
  rule<> assignment;
  rule<> ifThenElse;
  rule<> whileLoop;
  rule<> forLoop;
  rule<> breakExpression;
  rule<> let;
  rule<> multiplicativeExpression;
  rule<> primaryExpression;
  rule<> unaryExpression;
  rule<> booleanOr;
  rule<> booleanAnd;

  boost::spirit::qi::rule<Iterator> escapeCharacter;
};

} // namespace tiger
