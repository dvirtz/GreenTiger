#pragma once
#include "overload_set.h"
#include <boost/variant/apply_visitor.hpp>

namespace helpers {

namespace detail {

// a function object which calls one of the given overloads depending on the
// variants value
template <typename Variant> struct VariantMatcher {
  VariantMatcher(Variant &variant) : m_variant(variant) {}

  template <typename... Funcs> auto operator()(Funcs &&... fs) const {
    auto visitor = overload(std::forward<Funcs>(fs)...);
    return boost::apply_visitor(visitor, m_variant);
  }

  Variant &m_variant;
};

} // namespace detail

template <typename Variant> auto match(Variant &&variant) {
  return detail::VariantMatcher<Variant>{std::forward<Variant>(variant)};
}

template <typename Variant, typename... Types> void assertTypes(const Variant &variant) {
  bool result = false;
  (void)std::initializer_list<int>{(result |= (boost::get<Types>(&variant) != nullptr), 0)...};
  assert(result && "variant should have one of the listed types");
}

} // namespace helpers
