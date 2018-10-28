#pragma once
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace tiger {
namespace temp {

// a label is just a string
struct Label : type_safe::strong_typedef<Label, std::string>,
               type_safe::strong_typedef_op::equality_comparison<Label>,
               type_safe::strong_typedef_op::output_operator<Label> {
  using strong_typedef::strong_typedef;

  std::string& get() {
    return type_safe::get(*this);
  }

  const std::string& get() const {
    return type_safe::get(*this);
  }
};

using Labels = std::vector<temp::Label>;

} // namespace temp
} // namespace tiger

namespace std {
// we want to use it with the std::unordered_* containers
template <>
struct hash<tiger::temp::Label>
    : type_safe::hashable<tiger::temp::Label> {};
} // namespace std
