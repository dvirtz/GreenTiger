#pragma once
#include <tuple>
#include <vector>

namespace helpers {
namespace detail {

template <typename... Funcs, typename T, std::size_t... I>
void applyFunctionTupleToVector(const std::vector<T> &v,
                                const std::tuple<Funcs...> &funcs,
                                std::index_sequence<I...>) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif
  std::initializer_list<int>{(std::get<I>(funcs)(v[I]), 0)...};
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}
} // namespace detail

// apply function i to the i-th element of v
template <typename... Funcs, typename T>
void applyFunctionsToVector(const std::vector<T> &v, Funcs &&... funcs) {
  detail::applyFunctionTupleToVector(
      v, std::forward_as_tuple(funcs...),
      std::make_index_sequence<sizeof...(Funcs)>());
}

// apply function i to the i-th element of v
template <typename... Funcs, typename T>
void applyFunctionTupleToVector(const std::vector<T> &v,
                                const std::tuple<Funcs...> &funcs) {
  detail::applyFunctionTupleToVector(
      v, funcs, std::make_index_sequence<sizeof...(Funcs)>());
}
} // namespace helpers
