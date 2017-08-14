set(SOURCES Program.cpp Exercises.inl Test.cpp)
set(HEADERS Program.h Exercises.h)

add_executable(Chapter01 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter01 PRIVATE CATCH_CPP14_OR_GREATER)

target_link_libraries(Chapter01 PRIVATE Boost::boost Catch::Catch includeHeaders)

parse_unit_tests(Chapter01)