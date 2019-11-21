set(SOURCES Program.cpp Test.cpp SemanticAnalyzer.cpp TempMap.cpp EscapeAnalyser.cpp Translator.cpp Tree.cpp Canonicalizer.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h AbstractSyntaxTree.h 
  Annotation.h StringParser.h SemanticAnalyzer.h Types.h TempMap.h Frame.h CallingConvention.h EscapeAnalyser.h Translator.h Tree.h 
  Fragment.h  Canonicalizer.h)
list(APPEND SOURCES x64FastCall/Frame.cpp x64FastCall/CallingConvention.cpp)
list(APPEND HEADERS x64FastCall/Frame.h x64FastCall/Frame.cpp)

add_executable(Chapter08 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter08 PRIVATE CATCH_CPP14_OR_GREATER)

if(MSVC)
	set_source_files_properties(Program.cpp PROPERTIES COMPILE_FLAGS /bigobj)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  # https://stackoverflow.com/a/23376156/621176
  set_source_files_properties(Test.cpp PROPERTIES COMPILE_FLAGS -ftemplate-depth=1024)
endif()

target_link_libraries(Chapter08 
    PRIVATE 
        Boost::boost 
        Boost::filesystem 
        Boost::system
        Catch2::Catch2 
        includeHeaders
)

parse_unit_tests(Chapter08 --timeout 10)
