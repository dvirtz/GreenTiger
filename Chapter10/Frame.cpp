#include "Frame.h"
#include "Assembly.h"
#include <range/v3/view/concat.hpp>
#include <range/v3/view/single.hpp>

namespace tiger {
namespace frame {

Frame::Frame(temp::Map &tempMap, const CallingConvention &callingConvention) :
    m_tempMap{tempMap}, m_callingConvention{callingConvention} {}

ir::Statement Frame::procEntryExit1(const ir::Statement &body) const {
  return ir::Sequence{
    ranges::view::concat(m_parameterMoves, ranges::view::single(body))};
}

assembly::Instructions
  Frame::procEntryExit3(const assembly::Instructions &body) const {
  return body;
}

} // namespace frame
} // namespace tiger
