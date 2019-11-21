set(SOURCES m68kFrame.cpp m68kCallingConvention.cpp m68kCodeGenerator.cpp m68kMachine.cpp)
set(HEADERS m68kFrame.h m68kCallingConvention.h m68kCodeGenerator.h m68kRegisters.h m68kMachine.h)

add_library(${CHAPTER}_m68k ${HEADERS} ${SOURCES})

set_target_properties(${CHAPTER}_m68k PROPERTIES OUTPUT_NAME m68k)

target_include_directories(${CHAPTER}_m68k 
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${CONAN_TYPE_SAFE_ROOT}/include
    ${CONAN_GSL_MICROSOFT_ROOT}/include
)

target_link_libraries(${CHAPTER}_m68k 
  PRIVATE 
    includeHeaders 
    Boost::boost
    range-v3
)