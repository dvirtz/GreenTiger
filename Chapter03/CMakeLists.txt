set(SOURCES Program.cpp Test.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h)

add_executable(Chapter03 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter03 PRIVATE CATCH_CPP14_OR_GREATER $<$<CONFIG:Debug>:BOOST_SPIRIT_DEBUG>)

target_link_libraries(Chapter03 
    PRIVATE 
        Boost::boost 
        Boost::filesystem 
        Boost::system
        Catch2::Catch 
        includeHeaders)

parse_unit_tests(Chapter03)