#pragma once
#include "overload_set.h"
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>

namespace helpers {

template <typename... Variants> auto match(Variants &&... variants) {
  return [&](auto &&... funcs) {
    auto visitor = overload(std::forward<decltype(funcs)>(funcs)...);
    return boost::apply_visitor(visitor, std::forward<Variants>(variants)...);
  };
}

template <typename T, typename Variant> bool hasType(const Variant &variant) {
  return boost::get<T>(&variant) != nullptr;
}

template <typename Variant, typename... Types>
void assertTypes(const Variant &variant) {
  bool result = false;
  (void)std::initializer_list<int>{(result |= hasType<Types>(variant), 0)...};
  assert(result && "variant should have one of the listed types");
}

} // namespace helpers
