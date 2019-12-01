set(SOURCES Program.cpp Exercises.inl Test.cpp)
set(HEADERS Program.h Exercises.h)

add_executable(Chapter01 ${HEADERS} ${SOURCES})


target_link_libraries(Chapter01 PRIVATE Boost::boost Catch2::Catch2 includeHeaders)

parse_unit_tests(Chapter01)