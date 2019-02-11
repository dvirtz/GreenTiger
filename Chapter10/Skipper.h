#pragma once
#include "warning_suppress.h"
MSC_DIAG_OFF(4996 4459)
#include <boost/spirit/include/qi.hpp>
MSC_DIAG_ON()

namespace tiger {
template <typename Iterator>
class Skipper : public boost::spirit::qi::grammar<Iterator> {
public:
  Skipper() : Skipper::base_type(skip) {
    namespace ascii = boost::spirit::ascii;

    using ascii::char_;
    ascii::space_type space;

    skip = space                             // tab/space/cr/lf
           | "/*" >> *(char_ - "*/") >> "*/" // comments
      ;
  }

private:
  boost::spirit::qi::rule<Iterator> skip;
};
} // namespace tiger