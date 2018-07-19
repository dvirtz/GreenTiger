set(SOURCES x64Frame.cpp x64CallingConvention.cpp x64CodeGenerator.cpp x64Machine.cpp)
set(HEADERS x64Frame.h x64CallingConvention.h x64CodeGenerator.h x64Registers.h x64Machine.h)

add_library(x64 ${HEADERS} ${SOURCES})

target_include_directories(x64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(x64 
  PRIVATE 
    includeHeaders 
    Boost::boost
)