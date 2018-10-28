#pragma once

#include <boost/variant.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace tiger {

struct NilType {};

struct IntType {};

struct StringType {};

struct ArrayType;

struct RecordType;

struct NameType {
  std::string m_name;
};

struct VoidType {};

using Type =
  boost::variant<NilType, IntType, StringType,
                 boost::recursive_wrapper<ArrayType>,
                 boost::recursive_wrapper<RecordType>, NameType, VoidType>;

struct NamedType {
  std::string m_name;
  Type m_type;
};

struct ArrayType {
  NamedType m_elementType;
};

struct RecordType {
  struct RecordField {
    std::string m_name;
    NamedType m_type;
  };
  std::vector<RecordField> m_fields;
};

} // namespace tiger