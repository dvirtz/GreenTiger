set(SOURCES Program.cpp Test.cpp)
set(HEADERS Program.h)

add_executable(Chapter02 ${HEADERS} ${SOURCES})


target_link_libraries(Chapter02 
    PRIVATE 
        Boost::boost
        Boost::filesystem
        Boost::system
        Catch2::Catch2 
        includeHeaders)

parse_unit_tests(Chapter02)