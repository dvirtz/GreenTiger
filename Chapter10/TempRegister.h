#pragma once
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace tiger {
namespace temp {

// a register is just a number
struct Register : type_safe::strong_typedef<Register, int>,
                  type_safe::strong_typedef_op::equality_comparison<Register>,
                  type_safe::strong_typedef_op::relational_comparison<Register>,
                  type_safe::strong_typedef_op::output_operator<Register> {
  using strong_typedef::strong_typedef;
};

using Registers = std::vector<temp::Register>;

} // namespace temp
} // namespace tiger

namespace std {
// we want to use it with the std::unordered_* containers
template <>
struct hash<tiger::temp::Register>
    : type_safe::hashable<tiger::temp::Register> {};
} // namespace std
