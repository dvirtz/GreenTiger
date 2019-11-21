#pragma once
#include "warning_suppress.h"
#include <ostream>
MSC_DIAG_OFF(4913)
#include <range/v3/algorithm/copy.hpp>
MSC_DIAG_ON()
MSC_DIAG_OFF(4100)
#include <range/v3/view/intersperse.hpp>
MSC_DIAG_ON()
#include <range/v3/view/transform.hpp>
#include <range/v3/iterator/stream_iterators.hpp>

namespace helpers {
template <typename R, typename ElementToString>
void printRange(std::ostream &ost, const R &rng,
                ElementToString &&elementToString) {
  using namespace ranges;
  ost << "{";
  copy(rng | view::transform(std::forward<ElementToString>(elementToString))
         | view::intersperse(", "),
       ostream_iterator<std::string>(ost));
  ost << "}";
}

} // namespace helpers