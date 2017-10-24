#include "Frame.h"

namespace tiger {
namespace x64FastCall {
// implements https://docs.microsoft.com/en-gb/cpp/build/overview-of-x64-calling-conventions
class Frame : public tiger::Frame {
public:
  Frame(TempMap& tempMap, const Label& name, const BoolList& formals);
  // Inherited via Frame
  virtual Label name() const override;
  virtual AccessList formals() const override;
  virtual VariableAccess allocateLocal(bool escapes) override;

private:
  TempMap& m_tempMap;
  Label m_name;
  AccessList m_formals;
  static const int FRAME_INC = 4;
  int m_frameOffset = -FRAME_INC;
  // The __fastcall convention uses registers for the first four arguments
  static const size_t MAX_REGS = 4;
  size_t m_allocatedRegs = 0;
};
} // namespace x64FastCall
} // namespace tiger
