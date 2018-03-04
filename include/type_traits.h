#pragma once
#include <type_traits>
#include <iterator>

namespace helpers {

template <typename T> using void_t = void;

// check if type is a sequence
template<typename T, typename = void>
struct is_sequence : std::false_type {};

template<typename T>
struct is_sequence<T, void_t<typename T::value_type>> : std::true_type {};

// check if type is an iterator
template<typename T, typename = void>
struct is_iterator : std::false_type {};

template<typename T>
struct is_iterator<T, void_t<typename std::iterator_traits<T>::value_type>> : std::true_type {};

} // namespace helpers
