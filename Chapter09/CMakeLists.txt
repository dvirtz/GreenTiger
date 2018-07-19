macro(add_machine name)
  add_subdirectory(${name})
  list(APPEND MACHINE_LIBRARIES ${name})
endmacro()

add_machine(m68k)
add_machine(x64)

foreach(machine ${MACHINE_LIBRARIES})
  string(TOLOWER ${machine} machineLower)
  string(APPEND MACHINE_INCLUDES "#include \"${machine}/${machine}Machine.h\"\n")
  string(APPEND MACHINE_REGISTRATIONS "REGISTER_MACHINE(${machine}, tiger::${machineLower}::Machine)\n")
endforeach()

configure_file(MachineRegistration.h.in MachineRegistration.h)

set(SOURCES Program.cpp SemanticAnalyzer.cpp TempMap.cpp EscapeAnalyser.cpp Translator.cpp Tree.cpp Canonicalizer.cpp Assembly.cpp 
  CodeGenerator.cpp)
set(HEADERS Program.h ErrorHandler.h ExpressionParser.h Skipper.h IdentifierParser.h DeclerationParser.h AbstractSyntaxTree.h 
  Annotation.h StringParser.h SemanticAnalyzer.h Types.h TempMap.h Frame.h CallingConvention.h EscapeAnalyser.h Translator.h Tree.h 
  Fragment.h  Canonicalizer.h Assembly.h CodeGenerator.h MachineRegistrar.h)

add_library(Chapter09 ${HEADERS} ${SOURCES})

target_include_directories(Chapter09 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

if(MSVC)
	set_source_files_properties(Program.cpp PROPERTIES COMPILE_FLAGS /bigobj)
endif()

target_link_libraries(Chapter09 
  PRIVATE
    ${MACHINE_LIBRARIES}
  PUBLIC
    Boost::boost 
    includeHeaders
    range-v3::range-v3
)

add_subdirectory(test)
