#pragma once
#include <boost/spirit/include/qi.hpp>

namespace tiger {
template <typename Iterator>
class Skipper : public boost::spirit::qi::grammar<Iterator> {
public:
  Skipper() : Skipper::base_type(start) {
    namespace ascii = boost::spirit::ascii;

    using ascii::char_;
    ascii::space_type space;

    start = space                             // tab/space/cr/lf
            | "/*" >> *(char_ - "*/") >> "*/" // comments
      ;

    BOOST_SPIRIT_DEBUG_NODES((start))
  }

private:
  boost::spirit::qi::rule<Iterator> start;
};
} // namespace tiger