#pragma once
#include <type_traits>
#include <utility>

namespace helpers {

namespace detail {

// https://www.youtube.com/watch?v=mqei4JJRQ7s
// make an overload of multiple function objects
template <typename...> struct overload_set {
  void operator()() {}
};

template <typename Func, typename... Funcs>
struct overload_set<Func, Funcs...> : Func, overload_set<Funcs...> {
  using Func::operator();
  using overload_set<Funcs...>::operator();

  overload_set(const overload_set &) = default;
  overload_set &operator=(const overload_set &) = default;

  overload_set(const Func &f, const Funcs &... fs)
      : Func(f), overload_set<Funcs...>(fs...) {}
};

template <typename... Funcs>
auto
overload(Funcs &&... fs) {
  using os = overload_set<typename std::remove_reference<Funcs>::type...>;
  return os(std::forward<Funcs>(fs)...);
}

} // namespace detail

} // namespace helpers
