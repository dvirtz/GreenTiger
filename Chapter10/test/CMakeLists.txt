add_library(${CHAPTER}_testMain STATIC Test.h TestMain.cpp Test.cpp)

set_target_properties(${CHAPTER}_testMain PROPERTIES OUTPUT_NAME testMain)

target_link_libraries(${CHAPTER}_testMain PUBLIC 
  Catch2::Catch
  Microsoft.GSL::GSL
  Boost::filesystem
  Boost::system 
  includeHeaders
  ${CHAPTER}
)

function(add_chapter_test name)

  if(MSVC)
	  set_source_files_properties(${name}.cpp PROPERTIES COMPILE_FLAGS /bigobj)
  endif()

  add_executable(${CHAPTER}_${name} ${name}.cpp)

  set_target_properties(${CHAPTER}_${name} PROPERTIES OUTPUT_NAME ${name})

  target_link_libraries(${CHAPTER}_${name}
    PRIVATE
      ${CHAPTER}_testMain
  )
  
  parse_unit_tests(${CHAPTER}_${name} ARCHS m68k x64)
endfunction()

add_chapter_test(compileFiles)
add_chapter_test(lvalue)
add_chapter_test(for)
add_chapter_test(integer)
add_chapter_test(comparison)
add_chapter_test(typeDeclarations)
add_chapter_test(boolean)
add_chapter_test(array)
add_chapter_test(nil)
add_chapter_test(functionCall)
add_chapter_test(string)
add_chapter_test(assignment)
add_chapter_test(arithmetic)
add_chapter_test(ifThen)
add_chapter_test(while)
add_chapter_test(library)
add_chapter_test(record)
add_chapter_test(sequence)
add_chapter_test(functionDeclarations)
add_chapter_test(break)