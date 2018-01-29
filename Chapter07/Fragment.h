#pragma once
#include "Tree.h"
#include <memory>

namespace tiger {

namespace frame {
class Frame;
}

struct StringFragment {
  temp::Label m_label;
  std::string m_string;
};

struct FunctionFragment {
  ir::Statement m_body;
  std::shared_ptr<const frame::Frame> m_frame;
};

using Fragment = boost::variant<StringFragment, FunctionFragment>;
using FragmentList = std::vector<Fragment>;
} // namespace tiger
