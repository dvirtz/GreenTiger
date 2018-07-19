set(SOURCES Program.cpp Test.cpp Compiler.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h AbstractSyntaxTree.h Annotation.h
    StringParser.h Compiler.h Types.h)

add_executable(Chapter05 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter05 PRIVATE CATCH_CPP14_OR_GREATER)

if(MSVC)
	set_source_files_properties(Program.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

target_link_libraries(Chapter05 
    PRIVATE 
        Boost::boost 
        Boost::filesystem 
        Boost::system
        Catch2::Catch 
        includeHeaders)

parse_unit_tests(Chapter05)