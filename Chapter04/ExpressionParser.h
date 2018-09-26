#pragma once
#include "DeclerationParser.h"
#include "ErrorHandler.h"
#include "IdentifierParser.h"
#include "Skipper.h"
#include "StringParser.h"
#include "warning_suppress.h"
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_statement.hpp>
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()

namespace tiger {

template <typename Iterator>
class ExpressionParser
    : public boost::spirit::qi::grammar<Iterator, Skipper<Iterator>,
                                        tiger::ast::Expression()> {
public:
  ExpressionParser(ErrorHandler<Iterator> &errorHandler,
                   Annotation<Iterator> &annotation) :
      ExpressionParser::base_type(expression),
      identifier(errorHandler, annotation),
      declaration(*this, identifier, errorHandler, annotation),
      string(errorHandler, annotation) {
    namespace spirit  = boost::spirit;
    namespace qi      = spirit::qi;
    namespace ascii   = spirit::ascii;
    namespace phoenix = boost::phoenix;

    using namespace std::string_literals;

    using ascii::alnum;
    using phoenix::at_c;
    using qi::_1;
    using qi::_pass;
    using qi::_val;
    using qi::attr;
    using qi::eps;
    using qi::fail;
    using qi::lexeme;
    using qi::lit;
    using qi::on_error;
    using qi::on_success;
    using qi::repeat;
    using qi::skip;
    using qi::uint_;
    using qi::uint_parser;

    using namespace qi::labels;

    relationalOp.add("=", ast::Operation::EQUAL)("<>",
                                                 ast::Operation::NOT_EQUAL)(
      "<", ast::Operation::LESS_THEN)(">", ast::Operation::GREATER_THEN)(
      "<=", ast::Operation::LESS_EQUAL)(">=", ast::Operation::GREATER_EQUAL);

    additiveOp.add("+", ast::Operation::PLUS)("-", ast::Operation::MINUS);

    multiplicativeOp.add("*", ast::Operation::TIMES)("/",
                                                     ast::Operation::DIVIDE);

    unaryOp.add("-", ast::Operation::MINUS);

    wholeWord = lexeme[qi::string(_r1)
                       >> !(alnum | '_')]; // make sure we have whole words

    nil %= wholeWord("nil"s) >> attr(ast::NilExpression{});

    lvalue = identifier >> *(varField | subscript);

    varField = lit('.') > identifier;

    subscript = lit('[') > expression > ']';

    sequence = lit('(') >> -(expression % ';') > ')';

    integer = uint_;

    functionCall = identifier >> '(' > -(expression % ',') > ')';

    primaryExpression =
      nil | string | integer | functionCall | lvalue | sequence;

    unaryExpression =
      (primaryExpression > attr(ast::OperationExpressions{}))
      // -x is parsed as 0-x
      | (attr(ast::IntExpression{0}) >> repeat(1)[unaryOp > unaryExpression]);

    multiplicativeExpression =
      unaryExpression >> *(multiplicativeOp > unaryExpression);

    additiveExpression =
      multiplicativeExpression >> *(additiveOp > multiplicativeExpression);

    comparisonExpression = additiveExpression
                           // comparison operators do not associate
                           >> repeat(0, 1)[relationalOp > additiveExpression];

    booleanExpression = booleanAnd | booleanOr;

    booleanAnd = comparisonExpression >> '&'
                 > (booleanAnd | comparisonExpression)
                 > attr(ast::IntExpression{0});

    booleanOr = comparisonExpression >> '|' > attr(ast::IntExpression{1})
                > (booleanOr | comparisonExpression);

    record = identifier >> '{' > -((identifier > '=' > expression) % ',') > '}';

    array = identifier >> '[' >> expression >> ']' >> "of" >> expression;

    assignment = lvalue >> ":=" > expression;

    ifThenElse = wholeWord("if"s) >> expression > wholeWord("then"s)
                 > expression > -(wholeWord("else"s) > expression);

    whileLoop =
      wholeWord("while"s) > expression > wholeWord("do"s) > expression;

    forLoop = wholeWord("for"s) > identifier > ":=" > expression
              > wholeWord("to"s) > expression > wholeWord("do"s) > expression;

    breakExpression = wholeWord("break"s) >> attr(ast::BreakExpression{});

    let = wholeWord("let"s) > *declaration > wholeWord("in"s)
          > -(expression % ';') > wholeWord("end"s);

    expression = record | array | assignment | ifThenElse | whileLoop | forLoop
                 | breakExpression | let | booleanExpression
                 | comparisonExpression;

    on_error<fail>(expression,
                   phoenix::bind(errorHandler, "Error! Expecting "s, _4, _3));

    on_success(expression, phoenix::bind(annotation, _val, _1));

    BOOST_SPIRIT_DEBUG_NODES(
      (expression)(lvalue)(sequence)(integer)(functionCall)(additiveExpression)(
        comparisonExpression)(booleanExpression)(record)(array)(assignment)(
        ifThenElse)(whileLoop)(forLoop)(breakExpression)(let)(
        multiplicativeExpression)(primaryExpression)(unaryExpression)(
        booleanAnd)(booleanOr)(varField)(subscript)(nil))
  }

private:
  template <typename Signature = boost::spirit::qi::unused_type,
            typename Locals    = boost::spirit::qi::unused_type>
  using rule =
    boost::spirit::qi::rule<Iterator, Skipper<Iterator>, Signature, Locals>;

  IdentifierParser<Iterator> identifier;
  DeclerationParser<Iterator> declaration;
  StringParser<Iterator> string;

  rule<ast::Expression()> expression;
  rule<ast::VarExpression()> lvalue;
  rule<ast::ExpressionSequence()> sequence;
  rule<ast::IntExpression()> integer;
  rule<ast::CallExpression()> functionCall;
  rule<ast::ArithmeticExpression()> additiveExpression;
  rule<ast::ArithmeticExpression()> comparisonExpression;
  rule<ast::IfExpression()> booleanExpression;
  rule<ast::RecordExpression()> record;
  rule<ast::ArrayExpression()> array;
  rule<ast::AssignExpression()> assignment;
  rule<ast::IfExpression()> ifThenElse;
  rule<ast::WhileExpression()> whileLoop;
  rule<ast::ForExpression()> forLoop;
  rule<ast::BreakExpression()> breakExpression;
  rule<ast::LetExpression()> let;
  rule<ast::ArithmeticExpression()> multiplicativeExpression;
  rule<ast::Expression()> primaryExpression;
  rule<ast::ArithmeticExpression()> unaryExpression;
  rule<ast::IfExpression()> booleanOr;
  rule<ast::IfExpression()> booleanAnd;
  rule<ast::VarField()> varField;
  rule<ast::Subscript()> subscript;
  rule<ast::NilExpression()> nil;
  rule<void(std::string)> wholeWord;

  boost::spirit::qi::symbols<char, ast::Operation> relationalOp, additiveOp,
    multiplicativeOp, unaryOp;
};

} // namespace tiger