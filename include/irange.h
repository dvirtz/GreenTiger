#pragma once
#include <tuple>
#ifdef _MSC_VER
#include <boost/range/iterator_range.hpp>
#else
#include <range/v3/iterator_range.hpp>
#endif

namespace tiger {

template <typename I, typename S> constexpr auto irange(I begin, S end) {
#ifdef _MSC_VER
  return boost::make_iterator_range(begin, end);
#else
  return ranges::make_iterator_range(begin, end);
#endif
}

template <typename I, typename S> constexpr auto irange(std::pair<I, S> iterator_pair) {
  return tiger::irange(iterator_pair.first, iterator_pair.second);
}

} // namespace tiger