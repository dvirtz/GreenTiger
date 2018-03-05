set(SOURCES Program.cpp Test.cpp)
set(HEADERS Program.h)

add_executable(Chapter02 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter02 PRIVATE CATCH_CPP14_OR_GREATER)

target_link_libraries(Chapter02 
    PRIVATE 
        Boost::boost
        Boost::filesystem
        Boost::system
        Catch::Catch 
        includeHeaders)

parse_unit_tests(Chapter02)