set(SOURCES x64Frame.cpp x64CallingConvention.cpp x64CodeGenerator.cpp x64Machine.cpp)
set(HEADERS x64Frame.h x64CallingConvention.h x64CodeGenerator.h x64Registers.h x64Machine.h)

add_library(${CHAPTER}_x64 ${HEADERS} ${SOURCES})

set_target_properties(${CHAPTER}_x64 PROPERTIES OUTPUT_NAME x64)

target_include_directories(${CHAPTER}_x64 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(${CHAPTER}_x64 
  PRIVATE 
    includeHeaders 
    Boost::boost
    range-v3
)