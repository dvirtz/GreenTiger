#include "Frame.h"

namespace tiger {
namespace frame {
class CallingConvention {
public:
  CallingConvention(temp::Map& tempMap)
    : m_tempMap(tempMap)
  {}

  virtual ~CallingConvention() = default;

  virtual std::unique_ptr<Frame> createFrame(const temp::Label &name,
                                             const BoolList &formals) = 0;
  virtual int wordSize() const = 0;

  virtual temp::Register framePointer() const = 0;

  virtual temp::Register returnValue() const = 0;

  virtual ir::Expression accessFrame(const VariableAccess &access,
                                     const ir::Expression &framePtr) const = 0;

  virtual void allocateString(temp::Label label, const std::string& str) = 0;

  virtual ir::Expression externalCall(const std::string& name, const std::vector<ir::Expression>& args) = 0;

protected:
  temp::Map& m_tempMap;
};
} // namespace frame
} // namespace tiger
