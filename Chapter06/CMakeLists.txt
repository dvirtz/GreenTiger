set(SOURCES Program.cpp Test.cpp Compiler.cpp TempMap.cpp x64FastCallFrame.cpp EscapeAnalyser.cpp x64FastCallFrameFactory.cpp Translator.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h AbstractSyntaxTree.h Annotation.h
    StringParser.h Compiler.h Types.h TempMap.h Frame.h FrameFactory.h x64FastCallFrame.h EscapeAnalyser.h x64FastCallFrameFactory.h Translator.h)

add_executable(Chapter06 ${HEADERS} ${SOURCES})

target_compile_definitions(Chapter06 PRIVATE CATCH_CPP14_OR_GREATER)

if(MSVC)
	set_source_files_properties(Program.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

target_link_libraries(Chapter06 
    PRIVATE 
        Boost::boost 
        Boost::system
        Boost::filesystem 
        Catch::Catch 
        includeHeaders)

parse_unit_tests(Chapter06)