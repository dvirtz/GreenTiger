#pragma once
#include "overload_set.h"
#include <boost/fusion/include/accumulate.hpp>

namespace helpers {

namespace detail {

// a function object which calls one of the given overloads depending on the
// variants value
template <typename Sequence, typename State> struct FusionMatcher {
  FusionMatcher(Sequence &sequence, State initState)
      : m_sequence(sequence), m_initState(initState) {}

  template <typename... Funcs> auto operator()(Funcs &&... fs) const {
    auto visitor = overload(std::forward<Funcs>(fs)...);
    return boost::fusion::accumulate(m_sequence, m_initState, visitor);
  }

  Sequence &m_sequence;
  State m_initState;
};

} // namespace detail

template <typename Sequence, typename State>
auto accumulate(Sequence &&sequence, State initState) {
  return detail::FusionMatcher<Sequence, State>{
      std::forward<Sequence>(sequence), initState};
}

} // namespace helpers
