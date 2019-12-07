#pragma once
#include <ostream>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/view/intersperse.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/iterator/stream_iterators.hpp>

namespace helpers {
template <typename R, typename ElementToString>
void printRange(std::ostream &ost, const R &rng,
                ElementToString &&elementToString) {
  using namespace ranges;
  ost << "{";
  copy(rng | views::transform(std::forward<ElementToString>(elementToString))
         | views::intersperse(", "),
       ostream_iterator<std::string>(ost));
  ost << "}";
}

} // namespace helpers