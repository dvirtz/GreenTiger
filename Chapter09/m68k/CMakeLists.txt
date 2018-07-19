set(SOURCES m68kFrame.cpp m68kCallingConvention.cpp m68kCodeGenerator.cpp m68kMachine.cpp)
set(HEADERS m68kFrame.h m68kCallingConvention.h m68kCodeGenerator.h m68kRegisters.h m68kMachine.h)

add_library(m68k ${HEADERS} ${SOURCES})

target_include_directories(m68k PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(m68k 
  PRIVATE 
    includeHeaders 
    Boost::boost
)