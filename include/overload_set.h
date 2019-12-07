#pragma once
#include <type_traits>
#include <utility>
#include <boost/hana/functional/overload.hpp>

namespace helpers {

  namespace detail {

template<class... Ts> struct overload_set : Ts... { using Ts::operator()...; };
template<class... Ts> overload_set(Ts...) -> overload_set<Ts...>;

  }

template <typename... Funcs>
auto
overload(Funcs &&... fs) {
  return detail::overload_set{std::forward<Funcs>(fs)...};
}

} // namespace helpers
