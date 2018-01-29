#pragma once
#include <iostream>

namespace helpers {
template <typename T> struct strong_typedef {
  strong_typedef() = default;
  template<typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  constexpr explicit strong_typedef(U u) : m_val(std::move(u)) {}

  explicit operator T() const noexcept { return m_val; }

  friend std::ostream &operator<<(std::ostream &ost, const strong_typedef &sd) {
    return ost << sd.m_val;
  }

  friend bool operator==(const strong_typedef &lhs, const strong_typedef &rhs) {
    return lhs.m_val == rhs.m_val;
  }

  friend bool operator==(const strong_typedef &lhs, const T &rhs) {
    return lhs.m_val == rhs;
  }

private:
  T m_val;
};
} // namespace helpers

namespace std {
template <typename T> struct hash<helpers::strong_typedef<T>> {
  size_t operator()(const helpers::strong_typedef<T> &sd) const {
    return m_hash(static_cast<T>(sd));
  }

private:
  hash<T> m_hash;
};
} // namespace std
