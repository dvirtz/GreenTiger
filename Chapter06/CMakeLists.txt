set(SOURCES Program.cpp Test.cpp Compiler.cpp TempMap.cpp EscapeAnalyser.cpp Translator.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h AbstractSyntaxTree.h Annotation.h
    StringParser.h Compiler.h Types.h TempMap.h Frame.h CallingConvention.h EscapeAnalyser.h Translator.h)
list(APPEND SOURCES x64FastCall/Frame.cpp x64FastCall/CallingConvention.cpp)
list(APPEND HEADERS x64FastCall/Frame.h x64FastCall/Frame.cpp)

add_executable(Chapter06 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter06 PRIVATE CATCH_CPP14_OR_GREATER)

if(MSVC)
	set_source_files_properties(Program.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

target_link_libraries(Chapter06 
    PRIVATE 
        Boost::boost 
        Boost::filesystem 
        Boost::system
        Catch2::Catch 
        includeHeaders)

parse_unit_tests(Chapter06)